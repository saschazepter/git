#include "cache.h"
#include "alloc.h"
#include "attr.h"
#include "object-store.h"
#include "submodule-config.h"
#include "ll-merge.h"
#include "wrapper.h"
int git_diff_ui_config(const char *var, const char *value, void *cb)
		unsigned cm = parse_color_moved_ws(value);
		diff_context_default = git_config_int(var, value);
		diff_interhunk_context_default = git_config_int(var, value);
		diff_stat_graph_width = git_config_int(var, value);
	if (!strcmp(var, "diff.ignoresubmodules"))
			return -1;
	return git_diff_basic_config(var, value, cb);
int git_diff_basic_config(const char *var, const char *value, void *cb)
		diff_rename_limit_default = git_config_int(var, value);
		int val = parse_ws_error_highlight(value);
			return -1;
	return git_default_config(var, value, cb);
	 * Guarantee 3/8*16==6 for the graph part
	 * and 5/8*16==10 for the filename part
	options->a_prefix = "a/";
	options->b_prefix = "b/";
			strvec_push(&cmd.args, xfrm_msg);
	if (options->flags.follow_renames && options->pathspec.nr != 1)
		die(_("--follow requires exactly one pathspec"));
		OPT_BIT_F('s', "no-patch", &options->output_format,
			  N_("suppress diff output"),
			  DIFF_FORMAT_NO_OUTPUT, PARSE_OPT_NONEG),
		OPT_BIT_F(0, "raw", &options->output_format,
			  DIFF_FORMAT_RAW, PARSE_OPT_NONEG),
		OPT_BIT_F(0, "numstat", &options->output_format,
			  DIFF_FORMAT_NUMSTAT, PARSE_OPT_NONEG),
		OPT_BIT_F(0, "shortstat", &options->output_format,
			  DIFF_FORMAT_SHORTSTAT, PARSE_OPT_NONEG),
		OPT_CALLBACK_F('X', "dirstat", options, N_("<param1,param2>..."),
		OPT_CALLBACK_F(0, "dirstat-by-file", options, N_("<param1,param2>..."),
			       N_("synonym for --dirstat=files,param1,param2..."),
		OPT_BIT_F(0, "summary", &options->output_format,
			  DIFF_FORMAT_SUMMARY, PARSE_OPT_NONEG),
	if (output_format & DIFF_FORMAT_PATCH) {
		if (separator) {
			emit_diff_symbol(options, DIFF_SYMBOL_SEPARATOR, NULL, 0, 0);
			if (options->stat_sep)
				/* attach patch instead of inline */
				emit_diff_symbol(options, DIFF_SYMBOL_STAT_SEP,
						 NULL, 0, 0);
		}

		diff_flush_patch_all_file_pairs(options);
	}

	if (output_format & DIFF_FORMAT_CALLBACK)
		options->format_callback(q, options, options->format_callback_data);

int diff_result_code(struct diff_options *opt, int status)
	if (!opt->flags.exit_with_status &&
	    !(opt->output_format & DIFF_FORMAT_CHECKDIFF))
		return status;