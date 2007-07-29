EXTRA_DIST = turbulence-doc.doxygen \
	main-page-logo.png \
	body_back.gif \
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
	cp main-page-logo.png html
	cp turbulence.css html
	cp body_back.gif html
