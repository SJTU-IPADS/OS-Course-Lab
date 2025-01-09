{
  description = "A Nix-flake-based C/C++ development environment";

  inputs.nixpkgs.url = "https://flakehub.com/f/NixOS/nixpkgs/0.1.*.tar.gz";

  outputs =
    {
      self,
      nixpkgs,
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
      devShells = forEachSupportedSystem (
        { pkgs }:
        {
          default = pkgs.mkShell {
            buildInputs =
              with pkgs;
              [
                # For builds
                clang-tools
                cmake
                python3
                pkgsCross.aarch64-multiplatform.bintools
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
