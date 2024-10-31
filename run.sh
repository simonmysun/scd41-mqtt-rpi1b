set -o allexport; source "${BASH_SOURCE%/*}/.env"; set +o allexport

"${BASH_SOURCE%/*}/main"

