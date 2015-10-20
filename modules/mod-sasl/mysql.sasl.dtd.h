/**
 * C inline representation for DTD mysql.sasl.dtd, created by axl-knife
 */
#ifndef __MYSQL_SASL_DTD_H__
#define __MYSQL_SASL_DTD_H__
#define MYSQL_SASL_DTD "\n\
<!-- <sasl-auth-db> -->                                                                                                            \
<!ELEMENT sasl-auth-db (connection-settings, get-password, get-password-alt?, get-password-alt-cleanup?, auth-log?, ip-filter?)>   \
                                                                                                                                   \
<!-- <connection-settings> -->                                                                                                     \
<!ELEMENT connection-settings EMPTY>                                                                                               \
<!ATTLIST connection-settings                                                                                                      \
   user            CDATA   #REQUIRED                                                                                               \
   password        CDATA   #REQUIRED                                                                                               \
   database        CDATA   #REQUIRED                                                                                               \
   host            CDATA   #REQUIRED                                                                                               \
   port            CDATA   #IMPLIED>                                                                                               \
                                                                                                                                   \
<!-- <get-password> -->                                                                                                            \
<!ELEMENT get-password EMPTY>                                                                                                      \
<!ATTLIST get-password                                                                                                             \
   query           CDATA   #REQUIRED>                                                                                              \
                                                                                                                                   \
<!-- <get-password-alt> -->                                                                                                        \
<!ELEMENT get-password-alt EMPTY>                                                                                                  \
<!ATTLIST get-password-alt                                                                                                         \
   query           CDATA   #REQUIRED>                                                                                              \
                                                                                                                                   \
<!-- <get-password-alt-cleanup> -->                                                                                                \
<!ELEMENT get-password-alt-cleanup EMPTY>                                                                                          \
<!ATTLIST get-password-alt-cleanup                                                                                                 \
   query           CDATA   #REQUIRED>                                                                                              \
                                                                                                                                   \
<!-- <auth-log> -->                                                                                                                \
<!ELEMENT auth-log EMPTY>                                                                                                          \
<!ATTLIST auth-log                                                                                                                 \
   query           CDATA   #REQUIRED>                                                                                              \
                                                                                                                                   \
<!-- <ip-filter> -->                                                                                                               \
<!ELEMENT ip-filter EMPTY>                                                                                                         \
<!ATTLIST ip-filter                                                                                                                \
   query           CDATA   #REQUIRED>                                                                                              \
                                                                                                                                   \
                                                                                                                                   \
                                                                                                                                   \
                                                                                                                                   \
\n"
#endif
