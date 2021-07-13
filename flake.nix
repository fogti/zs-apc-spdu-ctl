{
  description = "APC SPDU control utility";
  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
    yz-flake-utils.url = "github:YZITE/flake-utils";
  };
  outputs = { nixpkgs, yz-flake-utils, ... }:
    yz-flake-utils.lib.mkFlakeFromProg {
      prevpkgs = nixpkgs;
      progname = "zs-apc-spdu-ctk";
      drvBuilder = final: prev: (import ./Cargo.nix {
        pkgs = final;
        defaultCrateOverrides = final.defaultCrateOverrides // {
          zs-apc-spdu-ctl = { ... }: {
            nativeBuildInputs = [ final.pkg-config ];
            buildInputs = [ final.net-snmp ];
            postPatch = ''
              substituteInPlace src/conf/host.rs \
                --replace /usr/sbin/fping "${final.fping}/bin/fping"
            '';
          };
        };
      }).rootCrate.build;
    };
}
