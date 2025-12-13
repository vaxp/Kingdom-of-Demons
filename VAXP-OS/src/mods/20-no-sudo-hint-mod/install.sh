set -e                  # exit on error
set -o pipefail         # exit on pipeline error
set -u                  # treat unset variable as error

print_ok "Removing the hint for sudo"

file=/etc/bash.bashrc

#if file exists
if [[ -f "$file" ]]; then
    print_ok "Removing the hint for sudo"
    if grep -q '^[[:space:]]*# sudo hint' "$file"; then
        sed -ri '/^[[:space:]]*# sudo hint/,/^fi[[:space:]]*$/ s/^/# /' "$file"
        judge "Remove the hint for sudo"
    else
        print_error "Error: 'sudo hint' not found in $file."
        exit 1
    fi
else
    print_error "Error: $file does not exist."
    exit 1
fi