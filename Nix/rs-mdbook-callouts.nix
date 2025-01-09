{
  lib,
  fetchFromGitHub,
  rustPlatform,
  pkg-config,
  openssl,
}:

rustPlatform.buildRustPackage rec {
  pname = "rs-mdbook-callouts";
  version = "0.2.3";

  src = fetchFromGitHub {
    owner = "ToolmanP";
    repo = pname;
    tag = "v${version}";
    hash = "sha256-HKSVAga8IHgaT+It8pv/RNWRt851IFDLhcWmc/3pm/g=";
  };

  cargoHash = "sha256-5/J7yDxLJLeDVQ/l4LmRCYQdTt6AELDdNJNZFGf8aTw=";

  buildInputs = [ openssl ];

  nativeBuildInputs = [ pkg-config ];

  meta = {
    description = "mdBook preprocessor to add Obsidian flavored callouts + Github alerts to your book";
    mainProgram = "mdbook-callouts";
    homepage = "https://github.com/ToolmanP/rs-mdbook-callouts";
    license = lib.licenses.mit;
    maintainers = with lib.maintainers; [
      definfo
    ];
  };
}
