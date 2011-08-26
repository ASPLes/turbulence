tmp_xml_examples = \
	global-settings.xml-tmp \
	clean-start.xml-tmp \
	on-bad-signal.xml-tmp \
	log-reporting.xml-tmp \
	tbc-modules.xml-tmp \
	module-conf.xml-tmp \
	path-def.xml-tmp \
	path-def-conf.xml-tmp \
	path-def-example.xml-tmp \
	template.xml-tmp \
	turbulence.python.xml-tmp \
	python.module.example.3.xml-tmp \
	python.module.example.4.xml-tmp \
	python.module.example.5.xml-tmp \
	module-skip.xml-tmp \
	override-system-paths.xml-tmp \
	tls-example.xml-tmp \
	tls-sasl-example.xml-tmp \
	notify-failures.xml-tmp \
	include-from-file.xml-tmp \
	include-from-dir.xml-tmp

xml_examples = \
	global-settings.xml \
	clean-start.xml \
	on-bad-signal.xml \
	log-reporting.xml \
	tbc-modules.xml \
	module-conf.xml \
	path-def.xml \
	path-def-conf.xml \
	path-def-example.xml \
	turbulence.python.xml \
	python.module.example.3.xml \
	python.module.example.4.xml \
	python.module.example.5.xml \
	module-skip.xml \
	override-system-paths.xml \
	tls-example.xml \
	tls-sasl-example.xml \
	notify-failures.xml \
	include-from-file.xml \
	include-from-dir.xml

drafts_and_docs = \
	draft-brosnan-beep-limit-close.txt \
	draft-brosnan-beep-servername-feature.txt


EXTRA_DIST = turbulence-doc.doxygen \
	main-page-logo.png \
	body_back.gif \
	../web/images/aspl-alt-compat-logo.png \
	turbulence-doc-footer.html \
	turbulence-doc-header.html \
	aspl-logo-header.png \
	draft-brosnan-beep-servername-feature.xml draft-brosnan-beep-servername-feature.txt \
	draft-brosnan-beep-limit-close.txt draft-brosnan-beep-limit-close.xml \
	python.module.example python.module.example.2 bg.gif aspl-alt-compat-logo.png \
	turbulence2.css $(xml_examples) $(tmp_xml_examples) $(drafts_and_docs)

bin_SCRIPTS = initial_build_doc

help:
	@echo 
	@echo " Documentation not build!:"
	@echo " ~~~~~~~~~~~~~~~~~~~~~~~~~"
	@echo " --> Doxygen documentation wasn't built because an stamp was detected: doc.stamp"
	@echo " --> To regenerate or force doc building type make build_doc or remove doc.stamp";
	@echo

# build drafts and xml rfcs
.xml.txt: 
	xml2rfc $<


initial_build_doc: 
	@echo "checking documenation stamp..";
	@test -e doc.stamp & make help
	@test -e doc.stamp || make build_doc
	@test -e doc.stamp || touch doc.stamp

build_doc: turbulence-config-location.txt clean_template template.xml $(tmp_xml_examples)
	@echo "Generating module documentation..";
	doxygen turbulence-doc.doxygen
	cp main-page-logo.png html/
	cp bg.gif html/
# cp ../web/turbulence.css html/
	cp turbulence2.css html/
	@test -d html/images || mkdir html/images
# 	cp ../web/images/bg.gif html/images/
# 	cp ../web/images/vcss.png html/images/
# 	cp ../web/images/url.png html/images/
# 	cp ../web/images/valid-html401.png html/images/
# 	cp body_back.gif html/
	cp aspl-alt-compat-logo.png html/
	cp aspl-logo-header.png html/

turbulence-config-location.txt:
	echo ">> turbulence --conf-location" > $@
	../src/turbulence --conf-location >> $@

.xml.xml-tmp:
	axl-knife -i $< -o $@ -e -p fragment

clean_template:
	rm -f template.xml
template.xml:
	../tools/tbc-mod-gen/tbc-mod-gen -t
