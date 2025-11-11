{
  description = "Multi-RTSP Grid Viewer (Qt6 + GStreamer) with IPC";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-24.05";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs =
    {
      self,
      nixpkgs,
      flake-utils,
    }:
    flake-utils.lib.eachDefaultSystem (
      system:
      let
        pkgs = import nixpkgs { inherit system; };
        gst = pkgs.gst_all_1;
        gstPlugins = with gst; [
          gst-plugins-base
          gst-plugins-good
          gst-plugins-bad
          gst-plugins-ugly
          gst-libav
          gst-vaapi
        ];
        gstPluginPath = pkgs.lib.makeSearchPathOutput "lib" "lib/gstreamer-1.0" (
          gstPlugins ++ [ gst.gstreamer ]
        );
      in
      {
        packages.default = pkgs.stdenv.mkDerivation {
          pname = "basestation-cameras";
          version = "0.1.0";
          src = ./.;

          nativeBuildInputs = with pkgs; [
            cmake
            ninja
            pkg-config
            qt6.wrapQtAppsHook
          ];

          buildInputs =
            with pkgs;
            [
              qt6.qtbase
              qt6.qttools
              qt6.qtnetworkauth
              qt6.qtwayland
              gst.gstreamer
              libunwind
              netcat-gnu
              parallel
            ]
            ++ gstPlugins;

          cmakeFlags = [
            "-G"
            "Ninja"
          ];

          postFixup = ''
            wrapQtApp $out/bin/basestation-cameras \
              --set QT_QPA_PLATFORM wayland \
              --set QT_WAYLAND_DISABLE_WINDOWDECORATION 1 \
              --set QT_STYLE_OVERRIDE Fusion \
              --prefix GST_PLUGIN_SYSTEM_PATH_1_0 : "${gstPluginPath}" \
              --prefix GST_PLUGIN_PATH_1_0 : "${gstPluginPath}" \
              --set LIBVA_DRIVER_NAME radeonsi \
              --prefix LIBVA_DRIVERS_PATH : ${pkgs.mesa.drivers}/lib/dri \
              --set GIO_MODULE_DIR ${pkgs.glib.bin}/lib/gio/modules \
              --prefix LD_LIBRARY_PATH : ${
                pkgs.lib.makeLibraryPath [
                  pkgs.libGL
                  pkgs.xorg.libX11
                  pkgs.xorg.libXi
                  pkgs.xorg.libXext
                  pkgs.xorg.libXrender
                  pkgs.xorg.libxcb
                  pkgs.libva
                ]
              }
          '';
        };

        apps.default = {
          type = "app";
          program = "${self.packages.${system}.default}/bin/basestation-cameras";
        };

        devShells.default = pkgs.mkShell {
          name = "basestation-cameras-dev";
          buildInputs =
            with pkgs;
            [
              cmake
              ninja
              pkg-config
              gcc
              gdb
              netcat-gnu
              parallel

              # Useful tools
              ffmpeg
              libunwind
            ]
            ++ (with qt6; [
              # Qt6
              qtbase
              qttools
              qtnetworkauth
            ])
            ++ (with gst; [
              # GStreamer + plugins
              gstreamer
              gst-plugins-base
              gst-plugins-good
              gst-plugins-bad
              gst-plugins-ugly
              gst-libav
              gst-vaapi
            ]);

          shellHook = ''
            export QT_LOGGING_RULES="*.debug=false;qt.qpa.*=false"
            export GST_DEBUG=1
            export GST_PLUGIN_SYSTEM_PATH_1_0="${gstPluginPath}"
            export GST_PLUGIN_PATH_1_0="${gstPluginPath}"
            export GIO_MODULE_DIR="${pkgs.glib.bin}/lib/gio/modules"
            export LIBVA_DRIVER_NAME=radeonsi
            export LIBVA_DRIVERS_PATH=${pkgs.mesa.drivers}/lib/dri
            export QT_WAYLAND_DISABLE_WINDOWDECORATION=1
          '';
        };
      }
    );
}
