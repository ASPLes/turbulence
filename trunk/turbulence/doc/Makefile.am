tmp_xml_examples = \
	global-settings.xml.tmp \
	clean-start.xml.tmp \
	on-bad-signal.xml.tmp \
	log-reporting.xml.tmp \
	tbc-modules.xml.tmp \
	module-conf.xml.tmp \
	path-def.xml.tmp \
	path-def-conf.xml.tmp \
	path-def-example.xml.tmp \
	template.xml.tmp 

xml_examples = \
	global-settings.xml \
	clean-start.xml \
	on-bad-signal.xml \
	log-reporting.xml \
	tbc-modules.xml \
	module-conf.xml \
	path-def.xml \
	path-def-conf.xml \
	path-def-example.xml


EXTRA_DIST = turbulence-doc.doxygen \
	main-page-logo.png \
	body_back.gif \
	../web/images/aspl-alt-compat-logo.png \
	turbulence-doc-footer.html \
	turbulence-doc-header.html \
	aspl-logo-header.png \
	turbulence2.css $(xml_examples) $(tmp_xml_examples)

bin_SCRIPTS = initial_build_doc

help:
	@echo 
	@echo " Documentation not build!:"
	@echo " ~~~~~~~~~~~~~~~~~~~~~~~~~"
	@echo " --> Doxygen documentation wasn't built because an stamp was detected: doc.stamp"
	@echo " --> To regenerate or force doc building type make build_doc or remove doc.stamp";
	@echo


#draft-brosnan-beep-limit-close.txt: draft-brosnan-beep-limit-close.xml
#	xml2rfc draft-brosnan-beep-limit-close.xml

initial_build_doc: 
	@echo "checking documenation stamp..";
	@test -e doc.stamp & make help
	@test -e doc.stamp || make build_doc
	@test -e doc.stamp || touch doc.stamp

build_doc: turbulence-config-location.txt clean_template template.xml $(tmp_xml_examples)
	@echo "Generating module documentation..";
	doxygen turbulence-doc.doxygen
	cp main-page-logo.png html/
	cp ../web/images/bg.gif html/
# cp ../web/turbulence.css html/
	cp turbulence2.css html/
	@test -d html/images || mkdir html/images
# 	cp ../web/images/bg.gif html/images/
# 	cp ../web/images/vcss.png html/images/
# 	cp ../web/images/url.png html/images/
# 	cp ../web/images/valid-html401.png html/images/
# 	cp body_back.gif html/
	cp ../web/images/aspl-alt-compat-logo.png html/
	cp aspl-logo-header.png html/

turbulence-config-location.txt:
	echo ">> turbulence --conf-location" > $@
	../src/turbulence --conf-location >> $@

%.xml.tmp: %.xml 
	axl-knife -i $< -o $@ -e -p fragment

clean_template:
	rm -f template.xml
template.xml:
	../tools/tbc-mod-gen/tbc-mod-gen -t
