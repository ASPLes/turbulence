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
# %find_lang %{name}

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
Requires: pcre-devel
%description  -n libturbulence
Turbulence is an extensible and configurable general purpose 
BEEP application server built on top of Vortex Library. 
It provides out of the box support for profile administration,
SASL users, system logging and more.
%files -n libturbulence
       
# libturbulence-dev package
%package -n libturbulence-dev
Summary: BEEP application server built on top of Vortex Library
Group: System Environment/Libraries
Requires: libaxl-dev
Requires: libvortex-1.1-dev
%description  -n libturbulence-dev
Development headers required to create turbulence modules or
tools.
%files -n libturbulence-dev

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

# libturbulence-mod-radmin package
%package -n libturbulence-mod-radmin
Summary: Remote admin support for Turbulence (mod-radmin)
Group: System Environment/Libraries
Requires: turbulence-server
%description  -n libturbulence-mod-radmin
A module that provides access to a remote administrative
interface to turbulence main server.
%files -n libturbulence-mod-radmin


# libturbulence-mod-sasl package
%package -n libturbulence-mod-sasl
Summary: SASL profile implementation for Turbulence
Group: System Environment/Libraries
Requires: turbulence-server
Requires: libvortex-sasl-1.1
%description  -n libturbulence-mod-sasl
Server side SASL module for Turbulence. BEEP authentication.
%files -n libturbulence-mod-sasl


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


# libturbulence-mod-tls package
%package -n libturbulence-mod-tls
Summary: TLS profile implementation for Turbulence
Group: System Environment/Libraries
Requires: libvortex-tls-1.1
Requires: turbulence-server
%description  -n libturbulence-mod-tls
Server side TLS module for Turbulence. BEEP session security.
%files -n libturbulence-mod-tls

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


# turbulence-utils package
%package -n turbulence-utils
Summary: Turbulence utils
Group: System Environment/Libraries
Requires: turbulence-server
Requires: python-vortex
Requires: python-vortex-sasl
Requires: python-vortex-tls
%description  -n turbulence-utils
Utils to manage turbulence features.
%files -n turbulence-utils




%changelog
* Sun Apr 07 2015 Francis Brosnan Blázquez <francis@aspl.es> - %{turbulence_version}
- New upstream release

