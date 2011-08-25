/**
 * C inline representation for DTD turbulence-config.dtd, created by axl-knife
 */
#ifndef __TURBULENCE_CONFIG_DTD_H__
#define __TURBULENCE_CONFIG_DTD_H__
#define TURBULENCE_CONFIG_DTD "\n\
<!-- main configuration setting -->                                                       \
<!ELEMENT turbulence (global-settings, modules, features?, profile-path-configuration)>   \
                                                                                          \
<!-- global-settings -->                                                                  \
<!ELEMENT global-settings (ports,                                                         \
                           listener,                                                      \
                           log-reporting,                                                 \
      tls-support,                                                                        \
      on-bad-signal,                                                                      \
      clean-start?,                                                                       \
      connections,                                                                        \
                    kill-childs-on-exit?,                                                 \
                    system-paths?,                                                        \
                    notify-failures?,                                                     \
                    allow-start-without-profiles?,                                        \
                    global-child-limit?)>                                                 \
                                                                                          \
<!ELEMENT ports           (port+)>                                                        \
<!ELEMENT port            (#PCDATA)>                                                      \
                                                                                          \
<!ELEMENT log-reporting (general-log, error-log, access-log, vortex-log) >                \
<!ATTLIST log-reporting enabled (yes|no) #REQUIRED>                                       \
                                                                                          \
<!ELEMENT general-log        EMPTY>                                                       \
<!ATTLIST general-log file   CDATA #REQUIRED>                                             \
                                                                                          \
<!ELEMENT error-log          EMPTY>                                                       \
<!ATTLIST error-log file     CDATA #REQUIRED>                                             \
                                                                                          \
<!ELEMENT access-log         EMPTY>                                                       \
<!ATTLIST access-log  file   CDATA #REQUIRED>                                             \
                                                                                          \
<!ELEMENT vortex-log         EMPTY>                                                       \
<!ATTLIST vortex-log  file   CDATA #REQUIRED>                                             \
                                                                                          \
<!ELEMENT tls-support       EMPTY>                                                        \
<!ATTLIST tls-support enabled (yes|no) #REQUIRED>                                         \
                                                                                          \
<!ELEMENT on-bad-signal       EMPTY>                                                      \
<!ATTLIST on-bad-signal action (hold|ignore|backtrace|quit|exit) #REQUIRED>               \
<!ATTLIST on-bad-signal mail-to CDATA #IMPLIED>                                           \
                                                                                          \
<!ELEMENT clean-start       EMPTY>                                                        \
<!ATTLIST clean-start   value  (yes|no) #REQUIRED>                                        \
<!ATTLIST on-bad-signal action (yes|no) #REQUIRED>                                        \
                                                                                          \
<!ELEMENT kill-childs-on-exit   EMPTY>                                                    \
<!ATTLIST kill-childs-on-exit   value  (yes|no) #REQUIRED>                                \
                                                                                          \
<!ELEMENT system-paths   (path*)>                                                         \
<!ELEMENT path        EMPTY>                                                              \
<!ATTLIST path        name  CDATA #REQUIRED                                               \
                      value CDATA #REQUIRED>                                              \
                                                                                          \
<!ELEMENT notify-failures   (smtp-server*)>                                               \
<!ELEMENT smtp-server        EMPTY>                                                       \
<!ATTLIST smtp-server        id         CDATA #REQUIRED                                   \
                             server     CDATA #REQUIRED                                   \
                             port       CDATA #REQUIRED                                   \
                             is-default CDATA #IMPLIED                                    \
                             mail-from  CDATA #IMPLIED                                    \
                             mail-to    CDATA #REQUIRED>                                  \
                                                                                          \
<!ELEMENT allow-start-without-profiles   EMPTY>                                           \
<!ATTLIST allow-start-without-profiles   value  (yes|no) #REQUIRED>                       \
                                                                                          \
<!ELEMENT global-child-limit   EMPTY>                                                     \
<!ATTLIST global-child-limit   value  CDATA #REQUIRED>                                    \
                                                                                          \
<!ELEMENT file-socket EMPTY>                                                              \
<!ATTLIST file-socket value  CDATA #REQUIRED                                              \
               mode   CDATA #IMPLIED                                                      \
               user   CDATA #IMPLIED                                                      \
               group  CDATA #IMPLIED>                                                     \
                                                                                          \
<!ELEMENT auth-config EMPTY>                                                              \
<!ATTLIST auth-config value CDATA #REQUIRED>                                              \
                                                                                          \
<!ELEMENT connections       (max-connections?)>                                           \
<!ELEMENT max-connections   EMPTY>                                                        \
<!ATTLIST max-connections   hard-limit CDATA #REQUIRED                                    \
                            soft-limit CDATA #REQUIRED>                                   \
                                                                                          \
                                                                                          \
<!-- modules -->                                                                          \
<!ELEMENT modules        (directory*, no-load?)>                                          \
                                                                                          \
<!ELEMENT directory       EMPTY>                                                          \
<!ATTLIST directory src   CDATA #REQUIRED>                                                \
                                                                                          \
<!ELEMENT module EMPTY>                                                                   \
<!ATTLIST module                                                                          \
   name   CDATA #REQUIRED>                                                                \
                                                                                          \
<!ELEMENT no-load (module*)>                                                              \
                                                                                          \
<!-- features -->                                                                         \
<!ELEMENT features       (request-x-client-close?)>                                       \
                                                                                          \
<!ELEMENT request-x-client-close       EMPTY>                                             \
<!ATTLIST request-x-client-close value (yes|no) #REQUIRED>                                \
                                                                                          \
<!-- listener to be started -->                                                           \
<!ELEMENT listener    (name+)>                                                            \
<!ELEMENT name        (#PCDATA)>                                                          \
                                                                                          \
<!-- profile-path-configuration support -->                                               \
<!ELEMENT profile-path-configuration  (path-def+)>                                        \
<!ELEMENT path-def        (if-success | allow)*>                                          \
                                                                                          \
<!ELEMENT if-success      (if-success | allow)*>                                          \
<!ELEMENT allow           (if-success | allow)*>                                          \
                                                                                          \
<!ATTLIST path-def                                                                        \
          path-name      CDATA #IMPLIED                                                   \
          server-name    CDATA #IMPLIED                                                   \
   src            CDATA #IMPLIED                                                          \
   dst            CDATA #IMPLIED                                                          \
   run-as-user    CDATA #IMPLIED                                                          \
   run-as-group   CDATA #IMPLIED                                                          \
   separate       CDATA #IMPLIED                                                          \
   child-limit    CDATA #IMPLIED                                                          \
   reuse          CDATA #IMPLIED                                                          \
   chroot         CDATA #IMPLIED                                                          \
          work-dir       CDATA #IMPLIED>                                                  \
                                                                                          \
<!ATTLIST if-success                                                                      \
   serverName   CDATA #IMPLIED                                                            \
          profile      CDATA #REQUIRED                                                    \
          connmark     CDATA #IMPLIED                                                     \
   max-per-conn CDATA #IMPLIED                                                            \
          preconnmark  CDATA #IMPLIED >                                                   \
                                                                                          \
<!ATTLIST allow                                                                           \
   serverName   CDATA #IMPLIED                                                            \
          profile      CDATA #REQUIRED                                                    \
   max-per-conn CDATA #IMPLIED                                                            \
          preconnmark  CDATA #IMPLIED >                                                   \
                                                                                          \
                                                                                          \
                                                                                          \
\n"
#endif
