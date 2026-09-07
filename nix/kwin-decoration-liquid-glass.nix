{
  lib,
  stdenv,
  cmake,
  kdePackages,
  src,
}:

stdenv.mkDerivation {
  pname = "kwin-decoration-liquid-glass";
  version = "unstable";
  src = "${src}/integrations/kwin/decoration-liquid-glass";

  nativeBuildInputs = [
    cmake
    kdePackages.extra-cmake-modules
  ];

  buildInputs = [
    kdePackages.kwin
    kdePackages.qtbase
    kdePackages.kconfig
    kdePackages.kcoreaddons
    kdePackages.kdecoration
  ];

  cmakeFlags = [ "-DCMAKE_BUILD_TYPE=Release" ];
  dontWrapQtApps = true;

  meta = with lib; {
    description = "KWin decoration plugin for liquid glass effect";
    homepage = "https://gitee.com/xiaoyintx_ciallo/test";
    license = licenses.gpl3;
    platforms = platforms.linux;
  };
}