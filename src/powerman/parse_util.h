#ifndef PM_PARSE_UTIL_H
#define PM_PARSE_UTIL_H

void conf_init(char *filename);
void conf_fini(void);

bool conf_addnodes(char *nodelist);
bool conf_node_exists(char *node);
hostlist_t conf_getnodes(void);

bool conf_get_use_tcp_wrappers(void);
void conf_set_use_tcp_wrappers(bool val);

int conf_get_plug_log_level(void);
void conf_set_plug_log_level(char *level);

List conf_get_listen(void);
void conf_add_listen(char *hostport);

void conf_exp_aliases(hostlist_t hl);
bool conf_add_alias(char *name, char *hosts);

#endif  /* PM_PARSE_UTIL_H */

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
