/* Copyright (C) 2012 by djomla :).

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1335 USA
 */

#include <my_global.h>
#include <sql_priv.h>
#include <stdlib.h>
#include <ctype.h>
#include <mysql_version.h>
#include <mysql/plugin.h>
#include <mysql/plugin_audit.h>
#include <my_dir.h>
#include "my_pthread.h" // pthread_handler_t
#include "my_sys.h"     // my_write, my_malloc
#include "m_string.h"   // strlen
#include "sql_plugin.h" // st_plugin_int
#include <sql_class.h>

/*
  Disable __attribute__() on non-gcc compilers.
*/
#if !defined(__attribute__) && !defined(__GNUC__)
#define __attribute__(A)
#endif


const char *sensitive_fields[]= {"email", "sallary", NULL};

void mask_sensitive_fields(char *query)
{
    
    for (int i= 0; sensitive_fields[i] != NULL; i++)
    {
        char new_query[1024];
        const char *field= sensitive_fields[i];
        char *pos= strstr(query, field);

        /*while (pos != NULL)
        {
            strncpy(pos, "****", strlen(field));
            pos = strstr(query, field);
        }*/

        if (pos)
        {
            strncpy(new_query, query, pos - query);
            new_query[pos - query]= '\0';
            strcat(new_query, "OBFUSCATE");
            strcat(new_query, "(");
            strcat(new_query, field);
            strcat(new_query, ")");
            strcat(new_query, pos + strlen(field));
            strcpy(query, new_query);
        }
    }
}

static void obfuscator(MYSQL_THD thd, unsigned int event_class,
                       const void *event)
{
    const struct mysql_event_general *event_general=
        (const struct mysql_event_general *) event;

    if (event_general->event_subclass == MYSQL_AUDIT_GENERAL_LOG)
    {
        if (strcmp(event_general->general_command, "Query") == 0)
        {
          sql_print_information("OBFUSCATE THIS QUERY: %s",
                                event_general->general_query);
            mask_sensitive_fields((char *) event_general->general_query);

            sql_print_information("OBFUSCATED QUERY: %s",
                                  event_general->general_query);
        }
    }
}

static int execute_query(MYSQL *mysql, const char *query)
{
    if (mysql_real_query(mysql, query, strlen(query)))
    {
        sql_print_error("OBFUSCATOR - Error executing query: %s",
                        mysql_error(mysql));
        return 1;
    }

    return 0;
}


static int obfuscator_init(void *p __attribute__((unused)))
{
    DBUG_ENTER("obfuscator_plugin_init");
    MYSQL *mysql = mysql_init(NULL);
    if (!mysql)
    {
        sql_print_error("OBFUSCATOR - Error initializing MySQL connection.");
        return 1;
    }

    if (mysql_real_connect_local(mysql) == NULL)
    {
        sql_print_error("OBFUSCATOR - Error connecting to MySQL.");
        mysql_close(mysql);
        return 1;
    }

    const char *create_table_query=
        "CREATE TABLE IF NOT EXISTS mysql.obfuscator_rules ( "
        "id INT PRIMARY KEY AUTO_INCREMENT, db_name VARCHAR(64),"
        "tbl_name VARCHAR(64), fld_name VARCHAR(64));";

    if (mysql_real_query(mysql, create_table_query,
                         (unsigned long)  strlen(create_table_query)))
    {
        sql_print_error("OBFUSCATOR - Error initializing rules table: %s",
                        mysql_error(mysql));
        mysql_close(mysql);
        return 1;
    }

    mysql_close(mysql);
    DBUG_RETURN(0);
    return 0;
}


static int obfuscator_deinit(void *p __attribute__((unused)))
{
    MYSQL *mysql= mysql_init(NULL);
    if (!mysql)
    {
        sql_print_error("OBFUSCATOR - Error initializing MySQL connection.");
        return 1;
    }

    if (mysql_real_connect_local(mysql) == NULL)
    {
      sql_print_error("OBFUSCATOR - Error connecting to MySQL.");
      mysql_close(mysql);
      return 1;
    }

    const char *drop_table_query= "DROP TABLE IF EXISTS obfuscator_rules";

    int result = execute_query(mysql, drop_table_query);
    mysql_close(mysql);
    return result;
}

struct st_mysql_audit obfuscator_plugin= {MYSQL_AUDIT_INTERFACE_VERSION,
                                          NULL,
                                          obfuscator,
                                          {MYSQL_AUDIT_GENERAL_CLASSMASK}};

maria_declare_plugin(obfuscator){
    MYSQL_AUDIT_PLUGIN,
    &obfuscator_plugin,
    "obfuscator",
    "Mladen Martincevic",
    "A plugin to mask designated field names in SELECT queries",
    PLUGIN_LICENSE_GPL,
    obfuscator_init,                     /* Plugin Init */
    obfuscator_deinit,                   /* Plugin Deinit */
    0x0100,                              /* 1.0 */
    NULL,                                /* status variables */
    NULL,                                /* system variables */
    "1.0",                               /* string version */
    MariaDB_PLUGIN_MATURITY_EXPERIMENTAL /* maturity */
} maria_declare_plugin_end;