{
  description = "A very basic flake";

  inputs = {
    nixpkgs = { 
      url = "github:NixOS/nixpkgs/nixos-unstable";
    };
    flake-utils.url = "github:numtide/flake-utils";
    esp-dev = {
      url = "github:mirrexagon/nixpkgs-esp-dev";
      inputs.nixpkgs.follows = "nixpkgs";
      inputs.flake-utils.follows = "flake-utils";
    };
  };

  outputs = { self, nixpkgs, flake-utils, esp-dev }: 
   flake-utils.lib.eachDefaultSystem (system:
    let
      mkShell = pkgs.mkShell;
      pkgs = import nixpkgs {
        inherit system;
        overlays = [esp-dev.overlays.default];
      };
      arduino-core = pkgs.fetchFromGitHub {
        owner = "esp8266";
        repo = "Arduino";
        rev = "3.1.2";
        hash = "sha256-NLOEYIEZX8Ae6vbwuqeU6Efk8Li0fSXaX3QVDyLhsQc="; #pkgs.lib.fakeHash;
      };
      gcc-xtensa-lx106-elf-bin = pkgs.callPackage "${esp-dev}/pkgs/esp8266-rtos-sdk/esp8266-toolchain-bin.nix" { };
      esp8266-rtos-sdk =  pkgs.callPackage "${esp-dev}/pkgs/esp8266-rtos-sdk/esp8266-rtos-sdk.nix" { };
    in
    {
      devShells.default = mkShell  {
        #nativeBuildInputs = [ esp8266 cc-xtensa-lx106-elf-bi arduino-core ];
        buildInputs = [ 
          arduino-core 
          gcc-xtensa-lx106-elf-bin
          esp8266-rtos-sdk
        ];

        shellHook = ''
        '';
      };
    }
  );
}
