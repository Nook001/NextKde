{
    lib,
    stdenv,
    cmake,
    kdePackages,
    src,
}:

stdenv.mkDerivation {
    pname = "kos-settings";
    version = "unstable";
    src = src;

    nativeBuildInputs = [ cmake ];

    buildInputs = [
        kdePackages.qtbase
        kdePackages.qtdeclarative
    ];

    dontBuild = true;

    installPhase = ''
        runHook preInstall

        # Build only apps/settings subdirectory
        cmake -S "${src}/apps/settings" -B build -G Ninja \
            -DCMAKE_BUILD_TYPE=Release \
            -DCMAKE_INSTALL_PREFIX=$out
        cmake --build build --parallel
        cmake --install build

        # Install QML files
        mkdir -p $out/share/kos/settings
        cp $src/apps/settings/main.qml $out/share/kos/settings/main.qml
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
