/**
 * C inline representation for DTD mod-turbulence.dtd, created by axl-knife
 */
#ifndef __MOD_TURBULENCE_DTD_H__
#define __MOD_TURBULENCE_DTD_H__
#define MOD_TURBULENCE_DTD "\n\
<!-- DTD to validate modules installed for turbulence -->   \
<!ELEMENT mod-turbulence (provides*)>                       \
<!ATTLIST mod-turbulence location CDATA #REQUIRED>          \
                                                            \
<!-- list of profiles that provides this module -->         \
<!ELEMENT provides (profile*)>                              \
                                                            \
<!-- list of profiles provided by a module -->              \
<!ELEMENT profile EMPTY>                                    \
<!ATTLIST profile value CDATA #REQUIRED>                    \
                                                            \
                                                            \
\n"
#endif
