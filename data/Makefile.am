dtddir = $(datadir)/turbulence
dtd_DATA = turbulence-config.dtd mod-turbulence.dtd turbulence-db-list.dtd

etcdir = $(sysconfdir)/turbulence
etc_DATA = turbulence.example.conf

EXTRA_DIST = $(dtd_DATA) $(etc_DATA)

install-exec-hook:
# create some directories
	test -d ${sysconfdir}/turbulence || mkdir ${sysconfdir}/turbulence
	test -d ${sysconfdir}/turbulence/profile.d || mkdir ${sysconfdir}/turbulence/profile.d

