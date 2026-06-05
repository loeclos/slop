{
  pkgs ? import <nixpkgs> { },
}:

pkgs.mkShell {
  name = "c-dev-shell";

  packages = with pkgs; [
    gnumake
    gdb
    valgrind
    libedit
  ];

  shellHook = ''
    echo "Slop development shell active."
  '';
}
