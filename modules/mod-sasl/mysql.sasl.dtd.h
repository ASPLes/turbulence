/**
 * C inline representation for DTD mysql.sasl.dtd, created by axl-knife
 */
#ifndef __MYSQL_SASL_DTD_H__
#define __MYSQL_SASL_DTD_H__
#define MYSQL_SASL_DTD "\n\
<!-- <sasl-auth-db>                                                                                                                                                             \
                                                                                                                                                                                \
     set-auth-id : 'yes' (default) or 'no'. Controls whether an identity                                                                                                        \
     produced by <auth-resolve> is published as the SASL auth id of the                                                                                                         \
     connection, which is what makes the mapping transparent to the rest                                                                                                        \
     of the stack. 'no' keeps the resolution available for %e, post-auth                                                                                                        \
     filters and notifications only. -->                                                                                                                                        \
<!-- NOTE the three extension declarations are a single repeatable choice,                                                                                                      \
     not three ordered groups. The backend reaches each family by name                                                                                                          \
     (axl_doc_get + axl_node_get_next_called), so document order BETWEEN                                                                                                        \
     different families is irrelevant to it, and forcing every auth-filter                                                                                                      \
     ahead of every auth-resolve only prevented writing the file in the                                                                                                         \
     order it is evaluated: a post-auth filter naturally reads after the                                                                                                        \
     resolve whose identity it checks. Order WITHIN a family is still                                                                                                           \
     meaningful and still preserved. -->                                                                                                                                        \
<!ELEMENT sasl-auth-db (connection-settings, get-password, get-password-alt?, get-password-alt-cleanup?, auth-log?, ip-filter?, (auth-filter | auth-resolve | auth-notify)*)>   \
<!ATTLIST sasl-auth-db                                                                                                                                                          \
   set-auth-id     CDATA   #IMPLIED>                                                                                                                                            \
                                                                                                                                                                                \
<!-- <connection-settings> -->                                                                                                                                                  \
<!ELEMENT connection-settings EMPTY>                                                                                                                                            \
<!ATTLIST connection-settings                                                                                                                                                   \
   user            CDATA   #REQUIRED                                                                                                                                            \
   password        CDATA   #REQUIRED                                                                                                                                            \
   database        CDATA   #REQUIRED                                                                                                                                            \
   host            CDATA   #REQUIRED                                                                                                                                            \
   port            CDATA   #IMPLIED>                                                                                                                                            \
                                                                                                                                                                                \
<!-- <get-password> -->                                                                                                                                                         \
<!ELEMENT get-password EMPTY>                                                                                                                                                   \
<!ATTLIST get-password                                                                                                                                                          \
   query           CDATA   #REQUIRED>                                                                                                                                           \
                                                                                                                                                                                \
<!-- <get-password-alt> -->                                                                                                                                                     \
<!ELEMENT get-password-alt EMPTY>                                                                                                                                               \
<!ATTLIST get-password-alt                                                                                                                                                      \
   query           CDATA   #REQUIRED>                                                                                                                                           \
                                                                                                                                                                                \
<!-- <get-password-alt-cleanup> -->                                                                                                                                             \
<!ELEMENT get-password-alt-cleanup EMPTY>                                                                                                                                       \
<!ATTLIST get-password-alt-cleanup                                                                                                                                              \
   query           CDATA   #REQUIRED>                                                                                                                                           \
                                                                                                                                                                                \
<!-- <auth-log> -->                                                                                                                                                             \
<!ELEMENT auth-log EMPTY>                                                                                                                                                       \
<!ATTLIST auth-log                                                                                                                                                              \
   query           CDATA   #REQUIRED>                                                                                                                                           \
                                                                                                                                                                                \
<!-- <ip-filter> -->                                                                                                                                                            \
<!ELEMENT ip-filter EMPTY>                                                                                                                                                      \
<!ATTLIST ip-filter                                                                                                                                                             \
   query           CDATA   #REQUIRED>                                                                                                                                           \
                                                                                                                                                                                \
                                                                                                                                                                                \
                                                                                                                                                                                \
                                                                                                                                                                                \
<!-- <auth-filter> : additional condition that may deny the auth.                                                                                                               \
     Repeatable, evaluated in document order. Generalises <ip-filter>.                                                                                                          \
                                                                                                                                                                                \
     stage : 'pre-auth' (default) or 'post-auth'                                                                                                                                \
     match : 'expression' (default), 'required' or 'forbidden' -->                                                                                                              \
<!ELEMENT auth-filter EMPTY>                                                                                                                                                    \
<!ATTLIST auth-filter                                                                                                                                                           \
   name            CDATA   #IMPLIED                                                                                                                                             \
   query           CDATA   #REQUIRED                                                                                                                                            \
   stage           CDATA   #IMPLIED                                                                                                                                             \
   match           CDATA   #IMPLIED>                                                                                                                                            \
                                                                                                                                                                                \
<!-- <auth-resolve> : maps the credential that authenticated onto the                                                                                                           \
     identity the session must run as. Repeatable, evaluated in document                                                                                                        \
     order, each one receiving the identity resolved so far in %e -->                                                                                                           \
<!ELEMENT auth-resolve EMPTY>                                                                                                                                                   \
<!ATTLIST auth-resolve                                                                                                                                                          \
   name            CDATA   #IMPLIED                                                                                                                                             \
   query           CDATA   #REQUIRED>                                                                                                                                           \
                                                                                                                                                                                \
<!-- <auth-notify> : fire and forget statement run once the auth result                                                                                                         \
     is known. Repeatable. Generalises <auth-log>.                                                                                                                              \
                                                                                                                                                                                \
     on : 'any' (default), 'ok' or 'failed' -->                                                                                                                                 \
<!ELEMENT auth-notify EMPTY>                                                                                                                                                    \
<!ATTLIST auth-notify                                                                                                                                                           \
   name            CDATA   #IMPLIED                                                                                                                                             \
   query           CDATA   #REQUIRED                                                                                                                                            \
   on              CDATA   #IMPLIED>                                                                                                                                            \
                                                                                                                                                                                \
\n"
#endif
