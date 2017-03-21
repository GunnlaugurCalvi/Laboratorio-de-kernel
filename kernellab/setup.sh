#!/bin/bash
SERVER="skel.ru.is"
SSH_FOLDER="/root/.ssh"
SSH_KEY="/root/.ssh/id_rsa"
SSH_CONFIG="/root/.ssh/config"
if [ -e .connected ]; then
    echo -n "You have already configured setup. Continue (Y/N):"
    read -n 1 answer
    if [[ ! ($answer =~ ^(Y|y)$) ]]; then
	echo
	exit 0
    fi
fi
rm -rf $SSH_FOLDER
echo
if [ ! -e $SSH_KEY ]; then
    ssh-keygen -P "" -f $SSH_KEY -q
fi
while :; do
    echo -n "Enter username: "
    read username
    username=$(echo $username | sed 's/ //g')
    if [ ! -z $username ]; then
	break
    fi
done
ssh-copy-id $username@$SERVER
if [ $? -eq 0 ]; then
    echo -n $username > .username
    cat <<EOF > $SSH_CONFIG
Host skel.ru.is
  Hostname skel.ru.is
  User $username
EOF
    echo "You can now run \"make handin\""
    touch .connected
else
    echo "Failed to connect to $SERVER using $username."
fi
