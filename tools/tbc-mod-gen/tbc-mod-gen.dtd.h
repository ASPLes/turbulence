/**
 * C inline representation for DTD tbc-mod-gen.dtd, created by axl-knife
 */
#ifndef __TBC_MOD_GEN_DTD_H__
#define __TBC_MOD_GEN_DTD_H__
#define TBC_MOD_GEN_DTD "\n\
<!ELEMENT mod-def (name, description, source-code)>                                         \
                                                                                            \
<!ATTLIST mod-def sources CDATA #IMPLIED>                                                   \
                                                                                            \
<!ELEMENT name (#PCDATA)>                                                                   \
                                                                                            \
<!ELEMENT description (#PCDATA)>                                                            \
                                                                                            \
<!ELEMENT source-code (additional-content?, init, close, reconf, unload, ppath-selected)>   \
                                                                                            \
<!ELEMENT additional-content (#PCDATA)>                                                     \
                                                                                            \
<!ELEMENT init (#PCDATA)>                                                                   \
                                                                                            \
<!ELEMENT close (#PCDATA)>                                                                  \
                                                                                            \
<!ELEMENT reconf (#PCDATA)>                                                                 \
                                                                                            \
<!ELEMENT unload (#PCDATA)>                                                                 \
                                                                                            \
<!ELEMENT ppath-selected (#PCDATA)>                                                         \
                                                                                            \
\n"
#endif
