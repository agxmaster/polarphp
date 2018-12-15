// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/12/12.

#include "CLI/CLI.hpp"
#include "polarphp/basic/adt/StringRef.h"
#include "polarphp/utils/InitPolar.h"
#include "polarphp/global/CompilerFeature.h"
#include "polarphp/global/Config.h"
#include "lib/ExecEnv.h"
#include "lib/LifeCycle.h"
#include "lib/Defs.h"
#include "lib/Commands.h"
#include "lib/ProcessTitle.h"

#include <vector>
#include <string>

using polar::basic::StringRef;

void setup_command_opts(CLI::App &parser);

static bool sg_showVersion;
static bool sg_showNgInfo;
static bool sg_interactive;
static bool sg_generateExtendInfo;
static bool sg_syntaxCheck;
static bool sg_showModulesInfo;
static bool sg_hideExternArgs;
static std::string sg_configPath{};
static std::string sg_scriptFilepath{};
static std::string sg_codeWithoutPhpTags{};
static std::string sg_beginCode{};
static std::string sg_everyLineExecCode{};
static std::string sg_everyLineExecScriptFilename{};
static std::string sg_endCode{};
static std::string sg_stripCodeFilename{};
static std::string sg_zendExtensionFilename{};
static std::vector<std::string> sg_scriptArgs{};
static std::vector<std::string> sg_defines{};
static std::string sg_reflectFunc{};
static std::string sg_reflectClass{};
static std::string sg_reflectModule{};
static std::string sg_reflectZendExt{};
static std::string sg_reflectConfig{};

int main(int argc, char *argv[])
{
   polar::InitPolar polarInitializer(argc, argv);
   CLI::App cmdParser;
   polarInitializer.initNgOpts(cmdParser);
   setup_command_opts(cmdParser);
   CLI11_PARSE(cmdParser, argc, argv);
   polar::ExecEnv execEnv = polar::retrieve_global_execenv();
   execEnv.setArgc(argc);
   execEnv.setArgv(argv);
#if defined(POLAR_OS_WIN32)
# ifdef PHP_CLI_WIN32_NO_CONSOLE
   int argc = __argc;
   char **argv = __argv;
# endif
   int num_args;
   wchar_t **argv_wide;
   char **argv_save = argv;
   BOOL using_wide_argv = 0;
#endif
   int exitStatus = SUCCESS;
   int moduleStarted = 0;
   std::string iniEntries;
   bool iniIgnore = 0;
   /*
    * Do not move this initialization. It needs to happen before argv is used
    * in any way.
    */
   argv = polar::save_ps_args(argc, argv);
#if defined(POLAR_OS_WIN32) && !defined(POLAR_CLI_WIN32_NO_CONSOLE)
   php_win32_console_fileno_set_vt100(STDOUT_FILENO, TRUE);
   php_win32_console_fileno_set_vt100(STDERR_FILENO, TRUE);
#endif
#if defined(POLAR_OS_WIN32) && defined(_DEBUG) && defined(POLAR_WIN32_DEBUG_HEAP)
   {
      int tmp_flag;
      _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
      _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDERR);
      _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
      _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);
      _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
      _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
      tmp_flag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
      tmp_flag |= _CRTDBG_DELAY_FREE_MEM_DF;
      tmp_flag |= _CRTDBG_LEAK_CHECK_DF;
      _CrtSetDbgFlag(tmp_flag);
   }
#endif

#ifdef HAVE_SIGNAL_H
#if defined(SIGPIPE) && defined(SIG_IGN)
   signal(SIGPIPE, SIG_IGN);
   /// ignore SIGPIPE in standalone mode so
   /// that sockets created via fsockopen()
   /// don't kill PHP if the remote site
   /// closes it.  in apache|apxs mode apache
   /// does that for us!  thies@thieso.net
   /// 20000419
#endif
#endif
   tsrm_startup(1, 1, 0, nullptr);
   (void)ts_resource(0);
   ZEND_TSRMLS_CACHE_UPDATE();
   zend_signal_startup();
#ifdef POLAR_OS_WIN32
   _fmode = _O_BINARY;			/*sets default for file streams to binary */
   setmode(_fileno(stdin), O_BINARY);		/* make the stdio mode be binary */
   setmode(_fileno(stdout), O_BINARY);		/* make the stdio mode be binary */
   setmode(_fileno(stderr), O_BINARY);		/* make the stdio mode be binary */
#endif
   /// processing pre module init command options
   if (!sg_defines.empty()) {
      polar::setup_init_entries_commands(iniEntries);
   }
   /// processing ini definitions
   ///
   execEnv.setIniDefaultsHandler(polar::cli_ini_defaults);
   execEnv.setPhpIniPathOverride(sg_configPath);
   execEnv.setPhpIniIgnoreCwd(true);
   execEnv.setPhpIniIgnore(iniIgnore);
   iniEntries += polar::HARDCODED_INI;

   execEnv.setInitEntries(iniEntries);
   /// startup after we get the above ini override se we get things right
   ///
   if (!polar::php_module_startup(nullptr, 0)) {
      // there is no way to see if we must call zend_ini_deactivate()
      // since we cannot check if EG(ini_directives) has been initialised
      // because the executor's constructor does not set initialize it.
      // Apart from that there seems no need for zend_ini_deactivate() yet.
      // So we goto out_err.
      exitStatus = 1;
      goto out;
   }
   moduleStarted = 1;
   /// module init finished
#if defined(POLAR_OS_WIN32)
   php_win32_cp_cli_setup();
   orig_cp = (php_win32_cp_get_orig())->id;
   /* Ignore the delivered argv and argc, read from W API. This place
      might be too late though, but this is the earliest place ATW
      we can access the internal charset information from PHP. */
   argv_wide = CommandLineToArgvW(GetCommandLineW(), &num_args);
   PHP_WIN32_CP_W_TO_ANY_ARRAY(argv_wide, num_args, argv, argc)
         using_wide_argv = 1;
   SetConsoleCtrlHandler(php_cli_win32_ctrl_handler, TRUE);
#endif
   /// processing options
   if (sg_showVersion) {
      polar::print_polar_version();
      return 0;
   }

   /* -e option */
   if (sg_generateExtendInfo) {
      CG(compiler_options) |= ZEND_COMPILE_EXTENDED_INFO;
   }
   zend_first_try {
      exitStatus = polar::dispatch_cli_command(cmdParser, argc, argv);
   } zend_end_try();

   /// module finished
   ///
out:
   if (moduleStarted) {
      polar::php_module_shutdown();
   }
   tsrm_shutdown();
#if defined(POLAR_OS_WIN32)
   (void)php_win32_cp_cli_restore();
   if (using_wide_argv) {
      PHP_WIN32_CP_FREE_ARRAY(argv, argc);
      LocalFree(argv_wide);
   }
   argv = argv_save;
#endif
   /// Do not move this de-initialization. It needs to happen right before
   /// exiting.
   ///
   polar::cleanup_ps_args(argv);
   exit(exitStatus);
   return 0;
}

void setup_command_opts(CLI::App &parser)
{
   parser.add_flag("-v, --version", sg_showVersion, "Show polarphp version info.");
   parser.add_flag("-i, --ng-info", sg_showNgInfo, "Show polarphp info.");
   parser.add_flag("-a, --interactive", sg_interactive, "Run interactively PHP shell.");
   parser.add_flag("-e, --generate-extend-info", sg_generateExtendInfo, "Generate extended information for debugger/profiler.");
   parser.add_flag("-l, --lint", sg_syntaxCheck, "Syntax check only (lint)");
   parser.add_flag("-m, --modules-info", sg_showModulesInfo, "Show compiled in modules.");
   parser.add_option("-c, --config", sg_configPath, "Look for php.yaml file in this directory.")->type_name("<path>|<file>");
   parser.add_option("-d", sg_defines, "Define INI entry foo with value 'bar'.")->type_name("foo[=bar]");
   parser.add_option("-f", sg_scriptFilepath, "Parse and execute <file>.")->type_name("<file>");
   parser.add_option("-r", sg_codeWithoutPhpTags, "Run PHP <code> without using script tags <?..?>.")->type_name("<code>");
   parser.add_option("-B", sg_beginCode, "Run PHP <begin_code> before processing input lines.")->type_name("<begin_code>");
   parser.add_option("-R", sg_everyLineExecCode, "Run PHP <code> for every input line.")->type_name("<code>");
   parser.add_option("-F", sg_everyLineExecScriptFilename, "Parse and execute <file> for every input line.")->type_name("<file>");
   parser.add_option("-E", sg_endCode, "Run PHP <end_code> after processing all input lines.")->type_name("<end_code>");
   parser.add_flag("-H", sg_hideExternArgs, "Hide any passed arguments from external tools.");
   parser.add_option("-w", sg_stripCodeFilename, "Output source with stripped comments and whitespace.")->type_name("<file>");
   parser.add_option("-z", sg_zendExtensionFilename, "Load Zend extension <file>.")->type_name("<file>");
   parser.add_option("args", sg_scriptArgs, "Arguments passed to script. Use -- args when first argument.")->type_name("string");
   parser.add_option("--rf", sg_reflectFunc, "Show information about function <name>.")->type_name("<name>");
   parser.add_option("--rc", sg_reflectClass, "Show information about class <name>.")->type_name("<name>");
   parser.add_option("--rm", sg_reflectModule, "Show information about extension <name>.")->type_name("<name>");
   parser.add_option("--rz", sg_reflectZendExt, "Show information about Zend extension <name>.")->type_name("<name>");
   parser.add_option("--ri", sg_reflectConfig, "Show configuration for extension <name>.")->type_name("<name>");
}
