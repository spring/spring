COMMAND=$1
shift

case "${COMMAND}" in

  build)
    . build.sh
    ;;

  dev)
    . dev.sh
    ;;

  shell)
    /bin/bash
    ;;

  *)
    echo "Invalid operation mode: $COMMAND" >&2
    exit 1
    ;;
esac