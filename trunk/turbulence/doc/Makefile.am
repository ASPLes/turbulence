EXTRA_DIST = turbulence-doc.doxygen \
	main-page-logo.png \
	body_back.gif \
	aspl-alt-compat-logo.png \
	turbulence-doc-footer.html \
	turbulence-doc-header.html \
	turbulence.css 

bin_SCRIPTS = initial_build_doc

help:
	@echo 
	@echo " Documentation not build!:"
	@echo " ~~~~~~~~~~~~~~~~~~~~~~~~~"
	@echo " --> Doxygen documentation wasn't built because an stamp was detected: doc.stamp"
	@echo " --> To regenerate or force doc building type make build_doc or remove doc.stamp";
	@echo


initial_build_doc:
	@echo "checking documenation stamp..";
	@test -e doc.stamp & make help
	@test -e doc.stamp || make build_doc
	@test -e doc.stamp || touch doc.stamp

build_doc:
	@echo "Generating module documentation..";
	doxygen turbulence-doc.doxygen
	cp main-page-logo.png html/
	cp ../web/turbulence.css html/
	cp turbulence2.css html/
	@test -d html/images || mkdir html/images
	cp ../web/images/bg.gif html/images/
	cp ../web/images/vcss.png html/images/
	cp ../web/images/url.png html/images/
	cp ../web/images/valid-html401.png html/images/
	cp body_back.gif html/
	cp aspl-alt-compat-logo.png html/
