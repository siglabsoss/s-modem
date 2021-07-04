# RapidCheck examples


# Test A loop
Assume you wrote the following loop.  The loop is decrementing a value 3 times based on bytes.  Lets assume we want to know what happens when this loop has random values for bytes

```c++
int bytes = 1; // focus on testing this
const int starting_fill = 1024;

int filled = starting_fill;


for(auto i = 0; i < 3; i++) {
    filled -= bytes;
}

```
## First test
We can write it like this.  This will choose a random value for `bytes` and then the loop run 3 times.  At the end we double check that the final value is `<` less than.

```c++
rc::check("check range of bytes", []() {

    // will randomize the value of bytes
    const auto bytes = *rc::gen::inRange(0, 8);
    const int starting_fill = 1024;

    int filled = starting_fill;


    for(auto i = 0; i < 3; i++) {
        filled -= bytes;
    }
    RC_ASSERT(  filled < starting_fill  );
});

```

## The problem
The problem with this test is that we allowed a value of 0 for bytes.  This will cause an error.

```bash
Using configuration: seed=10759555867265207845

- check range of bytes
Falsifiable after 1 tests

int:
0

/home/x/insidework/software_parent_repo/higgs_sdr_rev2/libs/s-modem/soapy/src/test/test_predict.cpp:52:
RC_ASSERT(filled < starting_fill)

Expands to:
1024 < 1024
Some of your RapidCheck properties had failures. To reproduce these, run with:
RC_PARAMS="reproduce=BQxYoV2YrBich52ZlBybmBiY5RXZzViDQFgDfGVll4AUB4wnRVZJOAVAO8ZUVWiDQFgDfGVlBAQAAAAAAAA"
```

## Re-run
Now if we re-run the code, we can focus on this failure.  it gives us a string

```bash
make -j test_ooo && RC_PARAMS="reproduce=BQxYoV2YrBich52ZlBybmBiY5RXZzViDQFgDfGVll4AUB4wnRVZJOAVAO8ZUVWiDQFgDfGVlBAQAAAAAAAA" ./test_ooo
```


## Re-run and Fix one way
We can fix the code by doing 2 things. we can either change the range to 1

```c++
rc::check("check range of bytes", []() {

    // will randomize the value of bytes
    const auto bytes = *rc::gen::inRange(1, 8); // CHANGE this
    const int starting_fill = 1024;

    int filled = starting_fill;


    for(auto i = 0; i < 3; i++) {
        filled -= bytes;
    }
    RC_ASSERT(  filled < starting_fill  );
});

```

## Re-run and Fix the other way
With this method we keep the range at 0, 8, however we check later on if we should continue to check.  This method could be more complicated and allow you to block off "exceptional cases" where you don't want to run your checks

```c++
rc::check("check range of bytes", []() {

    // will randomize the value of bytes
    const auto bytes = *rc::gen::inRange(0, 8);
    const int starting_fill = 1024;

    int filled = starting_fill;


    for(auto i = 0; i < 3; i++) {
        filled -= bytes;
    }

    RC_PRE(bytes != 0);  // ADD this

    RC_ASSERT(  filled < starting_fill  );
});

```

