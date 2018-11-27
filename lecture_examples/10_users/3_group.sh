# Create a new grop.
groupadd testgroup
tail -n 1 /etc/group

# Fill the group.
useradd -m sysproguser
useradd -m sysproguser2
usermod -aG testgroup sysproguser
usermod -aG testgroup sysproguser2

tail -n 3 /etc/group

# Cleanup.
groupdel testgroup
tail -n 3 /etc/group
userdel -r sysproguser
userdel -r sysproguser2
