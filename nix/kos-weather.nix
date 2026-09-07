{
  lib,
  stdenv,
  cmake,
  kdePackages,
  src,
}:

let
  # Build the Go data service separately using buildGoModule (handles vendoring)
  go-service = kdePackages.callPackage ./shell-data-service.nix { inherit src; };
in
stdenv.mkDerivation {
  pname = "kos-weather";
  version = "unstable";
  inherit src;

  nativeBuildInputs = [
    cmake
    kdePackages.extra-cmake-modules
    kdePackages.wrapQtAppsHook
  ];

  buildInputs = [
    kdePackages.qtbase
    kdePackages.qtdeclarative
  ];

  # Use a fake Go binary to skip the CMake data-service build step.
  # The real data service is built separately via buildGoModule.
  preConfigure = ''
    mkdir -p $TMPDIR/fake-bin
    cat > $TMPDIR/fake-bin/go <<'SCRIPT'
    #!/bin/sh
    echo "go: skipped (pre-built externally)"
    exit 0
    SCRIPT
    chmod +x $TMPDIR/fake-bin/go
    export PATH=$TMPDIR/fake-bin:$PATH
  '';

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

  # Install the real pre-built data service alongside the weather binary
  postInstall = ''
    mkdir -p $out/libexec
    ln -s ${go-service}/libexec/kos-data-service $out/libexec/kos-data-service
  '';

  meta = with lib; {
    description = "KOS Weather - standalone Qt Quick forecast application";
    homepage = "https://gitee.com/xiaoyintx_ciallo/test";
    license = licenses.gpl3;
    platforms = platforms.linux;
  };
}
