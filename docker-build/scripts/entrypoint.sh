COMMAND=$1
shift

case "${COMMAND}" in

  build)
    . build.sh
    ;;

  dev)
    . dev.sh
    ;;

  *)
    echo "Invalid operation mode: $1" >&2
    exit 1
    ;;
esac