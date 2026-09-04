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
    inherit src;

    nativeBuildInputs = [
        cmake
        ninja
        kdePackages.qtbase
        kdePackages.qtdeclarative
    ];

    dontBuild = true;
    dontConfigure = true;
    dontWrapQtApps = true;

    installPhase = ''
        runHook preInstall

        cmake -S "${src}/apps/settings" -B build -G Ninja \
            -DCMAKE_BUILD_TYPE=Release \
            -DCMAKE_INSTALL_PREFIX=$out
        cmake --build build --parallel
        cmake --install build

        mkdir -p $out/share/shared/qml
        cp -r $src/shared/qml/controls $out/share/shared/qml/controls

        runHook postInstall
    '';

    meta = with lib; {
        description = "KOS Desktop Shell settings application";
        homepage = "https://gitee.com/xiaoyintx_ciallo/test";
        license = licenses.gpl3;
        platforms = platforms.linux;
    };
}
