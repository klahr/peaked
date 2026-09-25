Name:       peaked

Summary:    Alcohol tracker that learns how you feel
Version:    0.1.0
Release:    1
License:    GPLv3
URL:        https://github.com/klahr/peaked
Source0:    %{name}-%{version}.tar.bz2
Requires:   sailfishsilica-qt5 >= 0.10.9
Requires:   nemo-qml-plugin-notifications-qt5
Requires:   libkeepalive
Requires:   qt5-qtgraphicaleffects
Requires:   qt5-qtdeclarative-import-multimedia
BuildRequires:  pkgconfig(sailfishapp) >= 1.0.2
BuildRequires:  pkgconfig(Qt5Core)
BuildRequires:  pkgconfig(Qt5Qml)
BuildRequires:  pkgconfig(Qt5Quick)
BuildRequires:  pkgconfig(Qt5DBus)
BuildRequires:  pkgconfig(Qt5Network)
BuildRequires:  desktop-file-utils

%description
Keeps track of the drinks you have had and estimates your blood alcohol
over time from your height, weight, age and sex. Asks how you feel as the
evening goes on, and shows how you usually feel at the level another drink
would take you to, when a pause would keep you lower and when to stop.
Asks the next morning how you feel and learns from which exposure, the
area under the curve, your mornings are usually rough. It never
recommends a drink.


%prep
%setup -q -n %{name}-%{version}

%build

%qmake5 

%make_build


%install
%qmake5_install


desktop-file-install --delete-original         --dir %{buildroot}%{_datadir}/applications                %{buildroot}%{_datadir}/applications/*.desktop

%files
%defattr(-,root,root,-)
%{_bindir}/%{name}
%{_datadir}/%{name}
%{_datadir}/applications/%{name}.desktop
%{_datadir}/icons/hicolor/*/apps/%{name}.png
