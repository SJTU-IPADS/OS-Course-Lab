{
  lib,
  fetchFromGitHub,
  rustPlatform,
  pkg-config,
  openssl,
}:

rustPlatform.buildRustPackage rec {
  pname = "mdbook-last-changed";
  version = "0.1.4";

  src = fetchFromGitHub {
    owner = "badboy";
    repo = pname;
    tag = "v${version}";
    hash = "sha256-5B0j/HjuN8jpTVldRR1e79VQxhhvIN0yCDqt9teEKUc=";
  };

  cargoHash = "sha256-aPIH4mzjuFvaGacskEfMI9Ufh1iak9gxshNLBb9xEbI=";

  buildInputs = [ openssl ];

  nativeBuildInputs = [ pkg-config ];

  meta = {
    description = "A preprocessor for mdbook to add a page's last change date and a link to the commit on every page";
    homepage = "https://github.com/badboy/mdbook-last-changed";
    license = lib.licenses.mit;
    maintainers = with lib.maintainers; [
      definfo
    ];
  };
}
