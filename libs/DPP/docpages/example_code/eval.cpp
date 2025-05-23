/**
 * D++ eval command example.
 * This is dangerous and for educational use only, here be dragons!
 */

#include <dpp/dpp.h>
#include <fmt/format.h>
#include <fstream>
#include <iostream>
/* We have to define this to make certain functions visible */
#ifndef _GNU_SOURCE
        #define _GNU_SOURCE
#endif
#include <link.h>
#include <dlfcn.h>
#include "eval.h"

/* This is an example function you can expose to your eval command */
int test_function() {
	return 42;
}

/* Important: This code is for UNIX-like systems only, e.g.
 * Linux, BSD, OSX. It will NOT work on Windows!
 * Note for OSX you'll probably have to change all references
 * from .so to .dylib.
 */
int main() {
	dpp::cluster bot("token", dpp::i_default_intents | dpp::i_message_content);

        bot.on_log(dpp::utility::cout_logger());

	/* This won't work in a slash command very well yet, as there is not yet
	 * a multi-line slash command input type.
	 */
	bot.on_message_create([&bot](const auto & event) {
		if (dpp::utility::utf8substr(event.msg.content, 0, 5) == "!eval") {

			/** 
			 * THIS IS CRITICALLY IMPORTANT!
			 * Never EVER make an eval command that isn't restricted to a specific developer by user id.
			 * With access to this command the person who invokes it has at best full control over
			 * your bot's user account and at worst, full control over your entire network!!!
			 * Eval commands are Evil (pun intended) and could even be considered a security
			 * vulnerability. YOU HAVE BEEN WARNED!
			 */
			if (event.msg.author.id != dpp::snowflake(MY_DEVELOPER)) {
				bot.message_create(dpp::message(event.msg.channel_id, "On the day i do this for you, Satan will be ice skating to work."));
				return;
			}

			/* We start by creating a string that contains a cpp program for a simple library.
			 * The library will contain one exported function called so_exec() that is called
			 * containing the raw C++ code to eval.
			 */
			std::string code = "#include <iostream>\n\
				#include <string>\n\
				#include <map>\n\
				#include <unordered_map>\n\
				#include <stdint.h>\n\
				#include <dpp/dpp.h>\n\
				#include <dpp/nlohmann/json.hpp>\n\
				#include <fmt/format.h>\n\
				#include \"eval.h\"\n\
				extern \"C\" void so_exec(dpp::cluster& bot, dpp::message_create_t event) {\n\
					" + dpp::utility::utf8substr(
						event.msg.content,
						6,
						dpp::utility::utf8len(event.msg.content)
					) + ";\n\
					return;\n\
				}";

			/* Next we output this string full of C++ to a cpp file on disk.
			 * This code assumes the current directory is writeable. The file will have a
			 * unique name made from the user's id and the message id.
			 */
        		std::string source_filename = std::to_string(event.msg.author.id) + "_" + std::to_string(event.msg.id) + ".cpp";
			std::fstream code_file(source_filename, std::fstream::binary | std::fstream::out);
			if (!code_file.is_open()) {
				bot.message_create(dpp::message(event.msg.channel_id, "Unable to create source file for `eval`"));
				return;
			}
			code_file << code;
			code_file.close();

			/* Now to actually compile the file. We use dpp::utility::exec to
			 * invoke a compiler. This assumes you are using g++, and it is in your path.
			 */
			double compile_start = dpp::utility::time_f();
			dpp::utility::exec("g++", {
				"-std=c++20",
				"-shared",	/* Build the output as a .so file */
				"-fPIC",
				std::string("-o") + std::to_string(event.msg.author.id) + "_" + std::to_string(event.msg.id) + ".so",
				std::to_string(event.msg.author.id) + "_" + std::to_string(event.msg.id) + ".cpp",
				"-ldpp",
				"-ldl"
			}, [event, &bot, source_filename, compile_start](const std::string &output) {

				/* After g++ is ran we end up inside this lambda with the output as a string */
				double compile_time = dpp::utility::time_f() - compile_start;

				/* Delete our cpp file, we don't need it any more */
				std::string del_file = std::string(getenv("PWD")) + std::to_string(event.msg.author.id) + "_" + std::to_string(event.msg.id) + ".cpp";
				unlink(del_file.c_str());

				/* On successful compilation g++ outputs nothing, so any output here is error output */
				if (output.length()) {
					bot.message_create(dpp::message(event.msg.channel_id, "Compile error: ```\n" + output + "\n```"));
				} else {
					
					/* Now for the meat of the function. To actually load
					 * our shared object we use dlopen() to load it into the
					 * memory space of our bot. If dlopen() returns a nullptr,
					 * the shared object could not be loaded. The user probably
					 * did something odd with the symbols inside their eval.
					 */
					std::string dl = std::string(getenv("PWD")) + std::to_string(event.msg.author.id) + "_" + std::to_string(event.msg.id) + ".so";
					auto shared_object_handle = dlopen(dl.c_str(), RTLD_NOW);
					if (!shared_object_handle) {
						const char *dlsym_error = dlerror();
						bot.message_create(dpp::message(event.msg.channel_id, "Shared object load error: ```\n" +
							std::string(dlsym_error ? dlsym_error : "Unknown error") +"\n```"));
						return;
					}

					/* This type represents the "void so_exec()" function inside
					 * the shared object library file.
					 */
					using function_pointer = void(*)(dpp::cluster&, dpp::message_create_t);

					/* Attempt to find the function called so_exec() inside the
					 * library we just loaded. If we can't find it, then the user
					 * did something really strange in their eval. Also note it's
					 * important we call dlerror() here to reset it before trying
					 * to use it a second time. It's weird-ass C code and is just
					 * like that.
					 */
					dlerror();
					function_pointer exec_run = (function_pointer)dlsym(shared_object_handle, "so_exec");
					const char *dlsym_error = dlerror();
					if (dlsym_error) {
						bot.message_create(dpp::message(event.msg.channel_id, "Shared object load error: ```\n" + std::string(dlsym_error) +"\n```"));
						dlclose(shared_object_handle);
						return;
					}

					/* Now we have a function pointer to our actual exec code in
					 * 'exec_run', so lets call it, and pass it a reference to
					 * the cluster, and also a copy of the message_create_t.
					 */
					double run_start = dpp::utility::time_f();
					exec_run(bot, event);
					double run_time = dpp::utility::time_f() - run_start;

					/* When we're done with a .so file we must always dlclose() it */
					dlclose(shared_object_handle);

					/* We are now done with the compiled code too */
					unlink(dl.c_str());

					/* Output some statistics */
					bot.message_create(dpp::message(event.msg.channel_id,
						"Execution completed. Compile time: " + std::to_string(compile_time) +
						"s, execution time " + std::to_string(run_time) + "s"));
				}
			});
		}
	});

	bot.start(dpp::st_wait);
	
	return 0;
}
