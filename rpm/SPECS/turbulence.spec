%define release_date %(date +"%a %b %d %Y")
%define turbulence_version %(cat VERSION)

Name:           turbulence
Version:        %{turbulence_version}
Release:        5%{?dist}
Summary:        BEEP application server built on top of Vortex Library
Group:          System Environment/Libraries
License:        LGPLv2+ 
URL:            http://www.aspl.es/turbulence
Source:         %{name}-%{version}.tar.gz

# BuildRequires:  libidn-devel
# BuildRequires:  krb5-devel
# BuildRequires:  libntlm-devel
# BuildRequires:  pkgconfig

%define debug_package %{nil}

%description
Turbulence is an extensible and configurable general purpose 
BEEP application server built on top of Vortex Library. 
It provides out of the box support for profile administration,
SASL users, system logging and more.


%prep
%setup -q

%build
PKG_CONFIG_PATH=/usr/lib/pkgconfig:/usr/local/lib/pkgconfig %configure --prefix=/usr --sysconfdir=/etc
make clean
make %{?_smp_mflags}

%install
make install DESTDIR=%{buildroot} INSTALL='install -p'
find %{buildroot} -name '*.la' -exec rm -f {} ';'
find %{buildroot} -name 'mod-test.*' -exec rm -f {} ';'
find %{buildroot} -name 'mod_test_10b.*' -exec rm -f {} ';'
find %{buildroot} -name 'mod_test_10_prev.*' -exec rm -f {} ';'
find %{buildroot} -name 'mod_test_11.*' -exec rm -f {} ';'
find %{buildroot} -name 'mod_test_15.*' -exec rm -f {} ';'
find %{buildroot} -name '*.conf.tmp' -exec rm -f {} ';'
find %{buildroot} -name '*.xml-tmp' -exec rm -f {} ';'
find %{buildroot} -name '*.win32.xml' -exec rm -f {} ';'
mkdir -p %{buildroot}/etc/init.d
install -p %{_builddir}/%{name}-%{version}/doc/turbulence-rpm-init.d %{buildroot}/etc/init.d/turbulence

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

# %files -f %{name}.lang
%doc AUTHORS COPYING NEWS README THANKS

# libturbulence package
%package -n libturbulence
Summary: BEEP application server built on top of Vortex Library
Group: System Environment/Libraries
Requires: libaxl1
Requires: libvortex-1.1
Requires: pcre
%description  -n libturbulence
Turbulence is an extensible and configurable general purpose 
BEEP application server built on top of Vortex Library. 
It provides out of the box support for profile administration,
SASL users, system logging and more.
%files -n libturbulence
   /usr/lib64/libturbulence.a
   /usr/lib64/libturbulence.so
   /usr/lib64/libturbulence.so.0
   /usr/lib64/libturbulence.so.0.0.0
       
# libturbulence-dev package
%package -n libturbulence-dev
Summary: BEEP application server built on top of Vortex Library
Group: System Environment/Libraries
Requires: libaxl-dev
Requires: libvortex-1.1
Requires: libvortex-1.1-dev
%description  -n libturbulence-dev
Development headers required to create turbulence modules or
tools.
%files -n libturbulence-dev
   /usr/include/turbulence/exarg.h
   /usr/include/turbulence/turbulence-child.h
   /usr/include/turbulence/turbulence-config.h
   /usr/include/turbulence/turbulence-conn-mgr.h
   /usr/include/turbulence/turbulence-ctx-private.h
   /usr/include/turbulence/turbulence-ctx.h
   /usr/include/turbulence/turbulence-db-list.h
   /usr/include/turbulence/turbulence-expr.h
   /usr/include/turbulence/turbulence-handlers.h
   /usr/include/turbulence/turbulence-log.h
   /usr/include/turbulence/turbulence-loop.h
   /usr/include/turbulence/turbulence-mediator.h
   /usr/include/turbulence/turbulence-moddef.h
   /usr/include/turbulence/turbulence-module.h
   /usr/include/turbulence/turbulence-ppath.h
   /usr/include/turbulence/turbulence-process.h
   /usr/include/turbulence/turbulence-run.h
   /usr/include/turbulence/turbulence-signal.h
   /usr/include/turbulence/turbulence-support.h
   /usr/include/turbulence/turbulence-types.h
   /usr/include/turbulence/turbulence.h
   /usr/lib/pkgconfig/sasl-radmin.pc
   /usr/share/mod-sasl/radmin/sasl-radmin.idl
   /usr/lib64/pkgconfig/turbulence.pc
   /usr/share/turbulence/mod-turbulence.dtd
   /usr/share/turbulence/tbc-mod-gen.dtd
   /usr/share/turbulence/turbulence-config.dtd
   /usr/share/turbulence/turbulence-db-list.dtd

# turbulence-server package
%package -n turbulence-server
Summary: BEEP application server built on top of Vortex Library
Group: System Environment/Libraries
Requires: libturbulence
%description  -n turbulence-server
Turbulence is an extensible and configurable general purpose 
BEEP application server built on top of Vortex Library. 
It provides out of the box support for profile administration,
SASL users, system logging and more.
%files -n turbulence-server
   /usr/bin/turbulence
   /usr/bin/turbulence-config
   /usr/bin/turbulence-ctl
   /etc/turbulence/turbulence.example.conf
   /etc/init.d/turbulence
%post -n turbulence-server
chkconfig turbulence on
if [ ! -f /etc/turbulence/turbulence.conf ]; then
        cp /etc/turbulence/turbulence.example.conf /etc/turbulence/turbulence.conf
fi
service turbulence restart

# libturbulence-mod-tunnel package
%package -n libturbulence-mod-tunnel
Summary: TUNNEL profile implementation for Turbulence
Group: System Environment/Libraries
Requires: turbulence-server
Requires: libvortex-tunnel-1.1
%description  -n libturbulence-mod-tunnel
Configurable TUNNEL profile for Turbulence: an appliation level
gateway for any BEEP profile.
%files -n libturbulence-mod-tunnel
   /usr/lib/turbulence/modules/mod-tunnel.a
   /usr/lib/turbulence/modules/mod-tunnel.so
   /usr/lib/turbulence/modules/mod-tunnel.so.0
   /usr/lib/turbulence/modules/mod-tunnel.so.0.0.0
   /etc/turbulence/mods-available/mod-tunnel.xml
   /etc/turbulence/tunnel/resolver.xml
   /etc/turbulence/tunnel/tunnel.conf

# libturbulence-mod-radmin package
%package -n libturbulence-mod-radmin
Summary: Remote admin support for Turbulence (mod-radmin)
Group: System Environment/Libraries
Requires: turbulence-server
%description  -n libturbulence-mod-radmin
A module that provides access to a remote administrative
interface to turbulence main server.
%files -n libturbulence-mod-radmin
   /usr/lib/turbulence/modules/mod_radmin.a
   /usr/lib/turbulence/modules/mod_radmin.so
   /usr/lib/turbulence/modules/mod_radmin.so.0
   /usr/lib/turbulence/modules/mod_radmin.so.0.0.0
   /usr/bin/tbc-setup-mod-radmin.py
   /etc/turbulence/mods-available/mod_radmin.xml

# libturbulence-mod-sasl package
%package -n libturbulence-mod-sasl
Summary: SASL profile implementation for Turbulence
Group: System Environment/Libraries
Requires: turbulence-server
Requires: libvortex-sasl-1.1, libvortex-xml-rpc-1.1
%description  -n libturbulence-mod-sasl
Server side SASL module for Turbulence. BEEP authentication.
%files -n libturbulence-mod-sasl
   /usr/lib/turbulence/modules/mod-sasl.a
   /usr/lib/turbulence/modules/mod-sasl.so
   /usr/lib/turbulence/modules/mod-sasl.so.0
   /usr/lib/turbulence/modules/mod-sasl.so.0.0.0
   /etc/turbulence/sasl/auth-db.example.xml
   /etc/turbulence/sasl/remote-admins.example.xml
   /etc/turbulence/sasl/sasl.example.conf
   /usr/bin/tbc-sasl-conf
   /etc/turbulence/mods-available/mod-sasl.xml


# libturbulence-mod-sasl-mysql package
%package -n libturbulence-mod-sasl-mysql
Summary: MySQL support for Turbulence SASL module
Group: System Environment/Libraries
Requires: libturbulence-mod-sasl
Requires: mysql-libs
Requires: turbulence-server
%description  -n libturbulence-mod-sasl-mysql
This provides MySQL support so it is possible to store and auth
users from a MySQL database.
%files -n libturbulence-mod-sasl-mysql
   /usr/lib/turbulence/modules/mod-sasl-mysql.a
   /usr/lib/turbulence/modules/mod-sasl-mysql.so
   /usr/lib/turbulence/modules/mod-sasl-mysql.so.0
   /usr/lib/turbulence/modules/mod-sasl-mysql.so.0.0.0
   /etc/turbulence/sasl/extension.example.modules


# libturbulence-mod-tls package
%package -n libturbulence-mod-tls
Summary: TLS profile implementation for Turbulence
Group: System Environment/Libraries
Requires: libvortex-tls-1.1
Requires: turbulence-server
%description  -n libturbulence-mod-tls
Server side TLS module for Turbulence. BEEP session security.
%files -n libturbulence-mod-tls
   /etc/turbulence/mods-available/mod_tls.xml
   /etc/turbulence/tls/tls.example.conf
   /usr/lib/turbulence/modules/mod_tls.a
   /usr/lib/turbulence/modules/mod_tls.so
   /usr/lib/turbulence/modules/mod_tls.so.0
   /usr/lib/turbulence/modules/mod_tls.so.0.0.0

# libturbulence-mod-websocket package
%package -n libturbulence-mod-websocket
Summary: BEEP over WebSocket support for Turbulence
Group: System Environment/Libraries
Requires: libvortex-websocket-1.1
Requires: turbulence-server
%description  -n libturbulence-mod-websocket
Administrative module and integration support for libvortex-websocket-1.1
into Turbulence.
%files -n libturbulence-mod-websocket
   /etc/turbulence/websocket/websocket.example.conf
   /etc/turbulence/mods-available/mod_websocket.xml
   /usr/lib/turbulence/modules/mod_websocket.a
   /usr/lib/turbulence/modules/mod_websocket.so
   /usr/lib/turbulence/modules/mod_websocket.so.0
   /usr/lib/turbulence/modules/mod_websocket.so.0.0.0


# libturbulence-mod-python package
%package -n libturbulence-mod-python
Summary: SASL profile implementation for Turbulence
Group: System Environment/Libraries
Requires: turbulence-server
Requires: python-vortex
Requires: python-vortex-sasl
Requires: python-vortex-tls
%description  -n libturbulence-mod-python
Server side SASL module for Turbulence. BEEP authentication.
%files -n libturbulence-mod-python
   /usr/lib/turbulence/modules/mod_python.a
   /usr/lib/turbulence/modules/mod_python.so
   /usr/lib/turbulence/modules/mod_python.so.0
   /usr/lib/turbulence/modules/mod_python.so.0.0.0
   /etc/turbulence/mods-available/mod-python.xml
   /etc/turbulence/python/python.example.conf


# turbulence-utils package
%package -n turbulence-utils
Summary: Turbulence utils
Group: System Environment/Libraries
Requires: turbulence-server
Requires: libvortex-1.1
Requires: python-vortex
Requires: python-vortex-sasl
Requires: python-vortex-tls
Requires: readline
%description  -n turbulence-utils
Utils to manage turbulence features.
%files -n turbulence-utils
   /usr/bin/tbc-dblist-mgr
   /usr/bin/tbc-mod-gen

%changelog
%include rpm/SPECS/changelog.inc

