{
  lib,
  stdenv,
  cmake,
  kdePackages,
  go,
  src,
}:

stdenv.mkDerivation {
  pname = "kos-weather";
  version = "unstable";
  inherit src;

  nativeBuildInputs = [
    cmake
    kdePackages.extra-cmake-modules
    go
  ];

  buildInputs = [
    kdePackages.qtbase
    kdePackages.qtdeclarative
  ];

  cmakeFlags = [
    "-DCMAKE_BUILD_TYPE=Release"
    "-DKOS_BUILD_PLATFORM=OFF"
    "-DKOS_BUILD_KWIN_PLUGINS=OFF"
    "-DKOS_BUILD_CALENDAR=OFF"
    "-DKOS_BUILD_TODO=OFF"
    "-DKOS_BUILD_WEATHER=ON"
    "-DKOS_BUILD_MUSIC=OFF"
    "-DBUILD_TESTING=OFF"
  ];

  preBuild = ''
    export GOPROXY=https://goproxy.cn,direct
  '';

  meta = with lib; {
    description = "KOS Weather - standalone Qt Quick forecast application";
    homepage = "https://gitee.com/xiaoyintx_ciallo/test";
    license = licenses.gpl3;
    platforms = platforms.linux;
  };
}
