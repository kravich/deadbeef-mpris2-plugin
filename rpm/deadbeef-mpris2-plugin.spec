# 
# Do NOT Edit the Auto-generated Part!
# Generated by: spectacle version 0.27
# 

Name:       deadbeef-mpris2-plugin

# >> macros
# << macros

Summary:    DeadBeef MPRIS2 plugin
Version:    1.12
Release:    2
Vendor:     meego
Group:      Applications/Multimedia
License:    GPLv2
URL:        https://github.com/Serranya/deadbeef-mpris2-plugin
Source100:  deadbeef-mpris2-plugin.yaml
Requires:   deadbeef >= 0.6.2
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(gio-2.0)
BuildRequires:  pkgconfig(gthread-2.0)
BuildRequires:  deadbeef-devel

%description
MPRIS2 plugin for DeadBeef media player

%prep
# >> setup
# << setup

%build
# >> build pre
# << build pre

%autogen --disable-static
%configure --disable-static
make %{?_smp_mflags}

# >> build post
# << build post

%install
rm -rf %{buildroot}
# >> install pre
# << install pre
%make_install

# >> install post
# << install post

%files
%defattr(-,root,root,-)
%{_libdir}/deadbeef/mpris.so
# >> files
# << files
