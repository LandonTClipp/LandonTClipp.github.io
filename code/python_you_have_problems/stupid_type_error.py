import random

def type_error() -> None:
    print("running function with type error")
    for i in 123:
        print(i)

def no_type_error() -> None:
    print("running function with no type error")
    for i in (0, 1, 2):
        print(i)

def main():
    random.seed()
    random_number = random.randrange(0, 99)
    if random_number % 7 == 0:
        type_error()
    else:
        no_type_error()

main()