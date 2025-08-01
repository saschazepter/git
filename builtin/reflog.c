#define USE_THE_REPOSITORY_VARIABLE

#include "builtin.h"
#include "config.h"
#include "gettext.h"
#include "revision.h"
#include "reachable.h"
#include "wildmatch.h"
#include "worktree.h"
#include "reflog.h"
#include "refs.h"
#include "parse-options.h"

#define BUILTIN_REFLOG_SHOW_USAGE \
	N_("git reflog [show] [<log-options>] [<ref>]")

#define BUILTIN_REFLOG_LIST_USAGE \
	N_("git reflog list")

#define BUILTIN_REFLOG_EXPIRE_USAGE \
	N_("git reflog expire [--expire=<time>] [--expire-unreachable=<time>]\n" \
	   "                  [--rewrite] [--updateref] [--stale-fix]\n" \
	   "                  [--dry-run | -n] [--verbose] [--all [--single-worktree] | <refs>...]")

#define BUILTIN_REFLOG_DELETE_USAGE \
	N_("git reflog delete [--rewrite] [--updateref]\n" \
	   "                  [--dry-run | -n] [--verbose] <ref>@{<specifier>}...")

#define BUILTIN_REFLOG_EXISTS_USAGE \
	N_("git reflog exists <ref>")

#define BUILTIN_REFLOG_DROP_USAGE \
	N_("git reflog drop [--all [--single-worktree] | <refs>...]")

static const char *const reflog_show_usage[] = {
	BUILTIN_REFLOG_SHOW_USAGE,
	NULL,
};

static const char *const reflog_list_usage[] = {
	BUILTIN_REFLOG_LIST_USAGE,
	NULL,
};

static const char *const reflog_expire_usage[] = {
	BUILTIN_REFLOG_EXPIRE_USAGE,
	NULL
};

static const char *const reflog_delete_usage[] = {
	BUILTIN_REFLOG_DELETE_USAGE,
	NULL
};

static const char *const reflog_exists_usage[] = {
	BUILTIN_REFLOG_EXISTS_USAGE,
	NULL,
};

static const char *const reflog_drop_usage[] = {
	BUILTIN_REFLOG_DROP_USAGE,
	NULL,
};

static const char *const reflog_usage[] = {
	BUILTIN_REFLOG_SHOW_USAGE,
	BUILTIN_REFLOG_LIST_USAGE,
	BUILTIN_REFLOG_EXPIRE_USAGE,
	BUILTIN_REFLOG_DELETE_USAGE,
	BUILTIN_REFLOG_DROP_USAGE,
	BUILTIN_REFLOG_EXISTS_USAGE,
	NULL
};

struct worktree_reflogs {
	struct worktree *worktree;
	struct string_list reflogs;
};

static int collect_reflog(const char *ref, void *cb_data)
{
	struct worktree_reflogs *cb = cb_data;
	struct worktree *worktree = cb->worktree;
	struct strbuf newref = STRBUF_INIT;

	/*
	 * Avoid collecting the same shared ref multiple times because
	 * they are available via all worktrees.
	 */
	if (!worktree->is_current &&
	    parse_worktree_ref(ref, NULL, NULL, NULL) == REF_WORKTREE_SHARED)
		return 0;

	strbuf_worktree_ref(worktree, &newref, ref);
	string_list_append_nodup(&cb->reflogs, strbuf_detach(&newref, NULL));

	return 0;
}

static int expire_unreachable_callback(const struct option *opt,
				 const char *arg,
				 int unset)
{
	struct reflog_expire_options *opts = opt->value;

	BUG_ON_OPT_NEG(unset);

	if (parse_expiry_date(arg, &opts->expire_unreachable))
		die(_("invalid timestamp '%s' given to '--%s'"),
		    arg, opt->long_name);

	opts->explicit_expiry |= REFLOG_EXPIRE_UNREACH;
	return 0;
}

static int expire_total_callback(const struct option *opt,
				 const char *arg,
				 int unset)
{
	struct reflog_expire_options *opts = opt->value;

	BUG_ON_OPT_NEG(unset);

	if (parse_expiry_date(arg, &opts->expire_total))
		die(_("invalid timestamp '%s' given to '--%s'"),
		    arg, opt->long_name);

	opts->explicit_expiry |= REFLOG_EXPIRE_TOTAL;
	return 0;
}

static int cmd_reflog_show(int argc, const char **argv, const char *prefix,
			   struct repository *repo UNUSED)
{
	struct option options[] = {
		OPT_END()
	};

	parse_options(argc, argv, prefix, options, reflog_show_usage,
		      PARSE_OPT_KEEP_DASHDASH | PARSE_OPT_KEEP_ARGV0 |
		      PARSE_OPT_KEEP_UNKNOWN_OPT);

	return cmd_log_reflog(argc, argv, prefix, the_repository);
}

static int show_reflog(const char *refname, void *cb_data UNUSED)
{
	printf("%s\n", refname);
	return 0;
}

static int cmd_reflog_list(int argc, const char **argv, const char *prefix,
			   struct repository *repo UNUSED)
{
	struct option options[] = {
		OPT_END()
	};
	struct ref_store *ref_store;

	argc = parse_options(argc, argv, prefix, options, reflog_list_usage, 0);
	if (argc)
		return error(_("%s does not accept arguments: '%s'"),
			     "list", argv[0]);

	ref_store = get_main_ref_store(the_repository);

	return refs_for_each_reflog(ref_store, show_reflog, NULL);
}

static int cmd_reflog_expire(int argc, const char **argv, const char *prefix,
			     struct repository *repo UNUSED)
{
	timestamp_t now = time(NULL);
	struct reflog_expire_options opts = REFLOG_EXPIRE_OPTIONS_INIT(now);
	int i, status, do_all, single_worktree = 0;
	unsigned int flags = 0;
	int verbose = 0;
	reflog_expiry_should_prune_fn *should_prune_fn = should_expire_reflog_ent;
	const struct option options[] = {
		OPT_BIT('n', "dry-run", &flags, N_("do not actually prune any entries"),
			EXPIRE_REFLOGS_DRY_RUN),
		OPT_BIT(0, "rewrite", &flags,
			N_("rewrite the old SHA1 with the new SHA1 of the entry that now precedes it"),
			EXPIRE_REFLOGS_REWRITE),
		OPT_BIT(0, "updateref", &flags,
			N_("update the reference to the value of the top reflog entry"),
			EXPIRE_REFLOGS_UPDATE_REF),
		OPT_BOOL(0, "verbose", &verbose, N_("print extra information on screen")),
		OPT_CALLBACK_F(0, "expire", &opts, N_("timestamp"),
			       N_("prune entries older than the specified time"),
			       PARSE_OPT_NONEG,
			       expire_total_callback),
		OPT_CALLBACK_F(0, "expire-unreachable", &opts, N_("timestamp"),
			       N_("prune entries older than <time> that are not reachable from the current tip of the branch"),
			       PARSE_OPT_NONEG,
			       expire_unreachable_callback),
		OPT_BOOL(0, "stale-fix", &opts.stalefix,
			 N_("prune any reflog entries that point to broken commits")),
		OPT_BOOL(0, "all", &do_all, N_("process the reflogs of all references")),
		OPT_BOOL(0, "single-worktree", &single_worktree,
			 N_("limits processing to reflogs from the current worktree only")),
		OPT_END()
	};

	git_config(reflog_expire_config, &opts);

	save_commit_buffer = 0;
	do_all = status = 0;

	argc = parse_options(argc, argv, prefix, options, reflog_expire_usage, 0);

	if (verbose)
		should_prune_fn = should_expire_reflog_ent_verbose;

	/*
	 * We can trust the commits and objects reachable from refs
	 * even in older repository.  We cannot trust what's reachable
	 * from reflog if the repository was pruned with older git.
	 */
	if (opts.stalefix) {
		struct rev_info revs;

		repo_init_revisions(the_repository, &revs, prefix);
		revs.do_not_die_on_missing_objects = 1;
		revs.ignore_missing = 1;
		revs.ignore_missing_links = 1;
		if (verbose)
			printf(_("Marking reachable objects..."));
		mark_reachable_objects(&revs, 0, 0, NULL);
		release_revisions(&revs);
		if (verbose)
			putchar('\n');
	}

	if (do_all) {
		struct worktree_reflogs collected = {
			.reflogs = STRING_LIST_INIT_DUP,
		};
		struct string_list_item *item;
		struct worktree **worktrees, **p;

		worktrees = get_worktrees();
		for (p = worktrees; *p; p++) {
			if (single_worktree && !(*p)->is_current)
				continue;
			collected.worktree = *p;
			refs_for_each_reflog(get_worktree_ref_store(*p),
					     collect_reflog, &collected);
		}
		free_worktrees(worktrees);

		for_each_string_list_item(item, &collected.reflogs) {
			struct expire_reflog_policy_cb cb = {
				.opts = opts,
				.dry_run = !!(flags & EXPIRE_REFLOGS_DRY_RUN),
			};

			reflog_expire_options_set_refname(&cb.opts,  item->string);
			status |= refs_reflog_expire(get_main_ref_store(the_repository),
						     item->string, flags,
						     reflog_expiry_prepare,
						     should_prune_fn,
						     reflog_expiry_cleanup,
						     &cb);
		}
		string_list_clear(&collected.reflogs, 0);
	}

	for (i = 0; i < argc; i++) {
		char *ref;
		struct expire_reflog_policy_cb cb = { .opts = opts };

		if (!repo_dwim_log(the_repository, argv[i], strlen(argv[i]), NULL, &ref)) {
			status |= error(_("reflog could not be found: '%s'"), argv[i]);
			continue;
		}
		reflog_expire_options_set_refname(&cb.opts, ref);
		status |= refs_reflog_expire(get_main_ref_store(the_repository),
					     ref, flags,
					     reflog_expiry_prepare,
					     should_prune_fn,
					     reflog_expiry_cleanup,
					     &cb);
		free(ref);
	}

	reflog_clear_expire_config(&opts);

	return status;
}

static int cmd_reflog_delete(int argc, const char **argv, const char *prefix,
			     struct repository *repo UNUSED)
{
	int i, status = 0;
	unsigned int flags = 0;
	int verbose = 0;

	const struct option options[] = {
		OPT_BIT('n', "dry-run", &flags, N_("do not actually prune any entries"),
			EXPIRE_REFLOGS_DRY_RUN),
		OPT_BIT(0, "rewrite", &flags,
			N_("rewrite the old SHA1 with the new SHA1 of the entry that now precedes it"),
			EXPIRE_REFLOGS_REWRITE),
		OPT_BIT(0, "updateref", &flags,
			N_("update the reference to the value of the top reflog entry"),
			EXPIRE_REFLOGS_UPDATE_REF),
		OPT_BOOL(0, "verbose", &verbose, N_("print extra information on screen")),
		OPT_END()
	};

	argc = parse_options(argc, argv, prefix, options, reflog_delete_usage, 0);

	if (argc < 1)
		return error(_("no reflog specified to delete"));

	for (i = 0; i < argc; i++)
		status |= reflog_delete(argv[i], flags, verbose);

	return status;
}

static int cmd_reflog_exists(int argc, const char **argv, const char *prefix,
			     struct repository *repo UNUSED)
{
	struct option options[] = {
		OPT_END()
	};
	const char *refname;

	argc = parse_options(argc, argv, prefix, options, reflog_exists_usage,
			     0);
	if (!argc)
		usage_with_options(reflog_exists_usage, options);

	refname = argv[0];
	if (check_refname_format(refname, REFNAME_ALLOW_ONELEVEL))
		die(_("invalid ref format: %s"), refname);
	return !refs_reflog_exists(get_main_ref_store(the_repository),
				   refname);
}

static int cmd_reflog_drop(int argc, const char **argv, const char *prefix,
			   struct repository *repo)
{
	int ret = 0, do_all = 0, single_worktree = 0;
	const struct option options[] = {
		OPT_BOOL(0, "all", &do_all, N_("drop the reflogs of all references")),
		OPT_BOOL(0, "single-worktree", &single_worktree,
			 N_("drop reflogs from the current worktree only")),
		OPT_END()
	};

	argc = parse_options(argc, argv, prefix, options, reflog_drop_usage, 0);

	if (argc && do_all)
		usage(_("references specified along with --all"));

	if (do_all) {
		struct worktree_reflogs collected = {
			.reflogs = STRING_LIST_INIT_DUP,
		};
		struct string_list_item *item;
		struct worktree **worktrees, **p;

		worktrees = get_worktrees();
		for (p = worktrees; *p; p++) {
			if (single_worktree && !(*p)->is_current)
				continue;
			collected.worktree = *p;
			refs_for_each_reflog(get_worktree_ref_store(*p),
					     collect_reflog, &collected);
		}
		free_worktrees(worktrees);

		for_each_string_list_item(item, &collected.reflogs)
			ret |= refs_delete_reflog(get_main_ref_store(repo),
						     item->string);
		string_list_clear(&collected.reflogs, 0);

		return ret;
	}

	for (int i = 0; i < argc; i++) {
		char *ref;
		if (!repo_dwim_log(repo, argv[i], strlen(argv[i]), NULL, &ref)) {
			ret |= error(_("reflog could not be found: '%s'"), argv[i]);
			continue;
		}

		ret |= refs_delete_reflog(get_main_ref_store(repo), ref);
		free(ref);
	}

	return ret;
}

/*
 * main "reflog"
 */
int cmd_reflog(int argc,
	       const char **argv,
	       const char *prefix,
	       struct repository *repository)
{
	parse_opt_subcommand_fn *fn = NULL;
	struct option options[] = {
		OPT_SUBCOMMAND("show", &fn, cmd_reflog_show),
		OPT_SUBCOMMAND("list", &fn, cmd_reflog_list),
		OPT_SUBCOMMAND("expire", &fn, cmd_reflog_expire),
		OPT_SUBCOMMAND("delete", &fn, cmd_reflog_delete),
		OPT_SUBCOMMAND("exists", &fn, cmd_reflog_exists),
		OPT_SUBCOMMAND("drop", &fn, cmd_reflog_drop),
		OPT_END()
	};

	argc = parse_options(argc, argv, prefix, options, reflog_usage,
			     PARSE_OPT_SUBCOMMAND_OPTIONAL |
			     PARSE_OPT_KEEP_DASHDASH | PARSE_OPT_KEEP_ARGV0 |
			     PARSE_OPT_KEEP_UNKNOWN_OPT);
	if (fn)
		return fn(argc - 1, argv + 1, prefix, repository);
	else
		return cmd_log_reflog(argc, argv, prefix, repository);
}
