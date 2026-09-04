{
    lib,
    stdenv,
    cmake,
    ninja,
    kdePackages,
    src,
}:

stdenv.mkDerivation {
    pname = "kos-settings";
    version = "unstable";
    src = "${src}/apps/settings";

    nativeBuildInputs = [
        cmake
        ninja
        kdePackages.qtbase
        kdePackages.qtdeclarative
    ];

    dontWrapQtApps = true;

    cmakeFlags = [ "-DCMAKE_BUILD_TYPE=Release" ];

    postInstall = ''
        mkdir -p $out/share/shared/qml
        cp -r ${src}/../../shared/qml/controls $out/share/shared/qml/controls
    '';

    meta = with lib; {
        description = "KOS Desktop Shell settings application";
        homepage = "https://gitee.com/xiaoyintx_ciallo/test";
        license = licenses.gpl3;
        platforms = platforms.linux;
    };
}
