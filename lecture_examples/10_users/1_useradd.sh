# Get root role.
sudo su

# Create a user and check that it worked.
useradd -m -c "comment" sysproguser
tail -n 1 /etc/passwd
tail -n 1 /etc/shadow

# Change password of a new user.
passwd sysproguser
su vladislav # your original user, before executing this script.
su sysproguser

# Delete the user
exit
userdel -r sysproguser
# If this command says that there are working processes, then
# kill them and repeat the command.
