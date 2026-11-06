import immutable_wrap as iw

user = iw.PyUser()


def print_details(immutable):
    print(immutable.id(), immutable.name().decode())


print("Soft immutable")
print_details(user.getSoftImmutable(b"foo1"))
print_details(user.getSoftImmutable(b"foo2"))

print("Hard immutable")
print_details(user.getHardImmutable(b"bar1"))
print_details(user.getHardImmutable(b"bar2"))

print("Hard immutable (shared ptr)")
print_details(user.getHardImmutableShared(b"goo1"))
print_details(user.getHardImmutableShared(b"goo2"))
