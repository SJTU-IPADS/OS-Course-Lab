{
  description = "OS-Course-Lab development environment";

  inputs.nixpkgs.url = "https://flakehub.com/f/NixOS/nixpkgs/0.1.*.tar.gz";
  inputs.flake-compat.url = "https://flakehub.com/f/edolstra/flake-compat/1.tar.gz";

  outputs =
    {
      self,
      nixpkgs,
      flake-compat,
    }:
    let
      supportedSystems = [
        "x86_64-linux"
        "aarch64-linux"
        "x86_64-darwin"
        "aarch64-darwin"
      ];
      forEachSupportedSystem =
        f:
        nixpkgs.lib.genAttrs supportedSystems (
          system:
          f {
            pkgs = import nixpkgs { inherit system; };
          }
        );
    in
    {
      # Run `nix develop . --command <desired_shell>`
      # or `nix develop .#mdbook --command <desired_shell>` to start a devshell
      #
      # nix-direnv is preferable to automatically load/unload the default devshell
      # when changing directory
      devShells = forEachSupportedSystem (
        { pkgs }:
        {
          default = pkgs.mkShell {
            buildInputs =
              with pkgs;
              [
                gnumake
                clang-tools
                cmake
                python3
                python3Packages.psutil
                pkgsCross.aarch64-multiplatform.bintools # objdump
                # docker/podman should be manually configured
              ]
              ++ lib.optionals stdenv.hostPlatform.isLinux [
                # macOS build only has the qemu-system files.
                qemu
                # Currently GDB is unsupported on aarch64-darwin.
                gdb
              ];
          };
          mdBook = pkgs.mkShell {
            buildInputs =
              let
                rs-mdbook-callouts = pkgs.callPackage ./Nix/rs-mdbook-callouts.nix { };
                mdbook-last-changed = pkgs.callPackage ./Nix/mdbook-last-changed.nix { };
              in
              with pkgs;
              [
                rs-mdbook-callouts
                mdbook
                mdbook-toc
                mdbook-last-changed
                mdbook-mermaid
              ];
          };
        }
      );
    };
}
