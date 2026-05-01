#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
ROOT_DIR=$(cd "${SCRIPT_DIR}/.." && pwd)
BUILD_DIR=${1:-build}
STAGING_DIR=${2:-${BUILD_DIR}/package}
DIST_DIR=${3:-dist}

COLOR_RESET='\e[0m'
COLOR_TITLE='\e[1;36m'
COLOR_INFO='\e[1;34m'
COLOR_SUCCESS='\e[1;32m'
COLOR_WARN='\e[1;33m'
COLOR_ERROR='\e[1;31m'

print_header() {
  printf "${COLOR_TITLE}%s${COLOR_RESET}\n" "========================================"
  printf "${COLOR_TITLE}%s${COLOR_RESET}\n" "  X4Launcher Packaging Menu"
  printf "${COLOR_TITLE}%s${COLOR_RESET}\n" "========================================"
  printf "${COLOR_INFO}Root: ${ROOT_DIR}${COLOR_RESET}\n"
  printf "${COLOR_INFO}Build dir: ${BUILD_DIR}${COLOR_RESET}\n"
  printf "${COLOR_INFO}Staging dir: ${STAGING_DIR}${COLOR_RESET}\n"
  printf "${COLOR_INFO}Output dist: ${DIST_DIR}${COLOR_RESET}\n"
}

print_menu() {
  cat <<EOF

${COLOR_TITLE}Seleccione una opción:${COLOR_RESET}
  1) Compilar release
  2) Recolectar dependencias
  3) Limpiar símbolos del paquete
  4) Fijar RPATH relativo
  5) Verificar con ldd
  6) Probar en entorno mínimo
  7) Crear paquete Linux tar.gz
  8) Crear paquete Windows zip
  9) Flujo completo (1→8)
  0) Salir
EOF
}

run_step() {
  echo
  printf "${COLOR_INFO}>> Ejecutando: %s${COLOR_RESET}\n" "$*"
  "$@"
}

confirm_continue() {
  printf "${COLOR_WARN}Presione Enter para continuar...${COLOR_RESET}" 
  read -r _
}

check_tool() {
  if ! command -v "$1" >/dev/null 2>&1; then
    printf "${COLOR_WARN}Aviso: '%s' no está instalado. Algunas opciones pueden fallar.${COLOR_RESET}\n" "$1"
  fi
}

build_release() {
  run_step bash "${SCRIPT_DIR}/build_release.sh" "${BUILD_DIR}" "${STAGING_DIR}"
}

collect_deps() {
  run_step bash "${SCRIPT_DIR}/collect_deps.sh" "${STAGING_DIR}" "${DIST_DIR}"
}

strip_binaries() {
  run_step bash "${SCRIPT_DIR}/strip_binaries.sh" "${DIST_DIR}"
}

fix_rpath() {
  run_step bash "${SCRIPT_DIR}/fix_rpath.sh" "${DIST_DIR}/bin/X4Launcher" "${DIST_DIR}/lib"
}

check_ldd() {
  run_step bash "${SCRIPT_DIR}/check_ldd.sh" "${DIST_DIR}"
}

test_env() {
  run_step bash "${SCRIPT_DIR}/test_env.sh" "${DIST_DIR}"
}

package_linux() {
  run_step bash "${SCRIPT_DIR}/package_linux.sh" "${DIST_DIR}" "${ROOT_DIR}/X4Launcher-linux.tar.gz"
}

package_windows() {
  run_step bash "${SCRIPT_DIR}/package_windows.sh" "${DIST_DIR}" "${ROOT_DIR}/X4Launcher-windows.zip"
}

full_flow() {
  build_release
  collect_deps
  strip_binaries
  fix_rpath
  check_ldd
  test_env
  package_linux
  package_windows
}

main() {
  check_tool patchelf
  check_tool zip
  check_tool strip

  while true; do
    clear
    print_header
    print_menu
    printf "${COLOR_INFO}Opción: ${COLOR_RESET}"
    read -r choice

    case "${choice}" in
      1) build_release ;;
      2) collect_deps ;;
      3) strip_binaries ;;
      4) fix_rpath ;;
      5) check_ldd ;;
      6) test_env ;;
      7) package_linux ;;
      8) package_windows ;;
      9) full_flow ;;
      0) printf "${COLOR_SUCCESS}Salir.¡Hasta luego!${COLOR_RESET}\n" ; break ;;
      *) printf "${COLOR_ERROR}Opción no válida: %s${COLOR_RESET}\n" "${choice}" ;;
    esac

    confirm_continue
  done
}

main "$@"
