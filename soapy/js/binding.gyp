{
  'targets': [

    {
      'target_name': 'ExampleChain'
      ,'cflags': [ '-Werror=return-type' ]
      ,'cflags_cc': [ '-Werror=return-type' ]

      ,'sources':[
       'cpp/examplechain.cpp'
      ,'cpp/bevstream/BevPair2.cpp'
      ,'cpp/bevstream/BevStream.cpp'
      ,'cpp/bevstream/GainStream.cpp'
      ,'cpp/bevstream/ToJs.cpp'
      ]

      ,'include_dirs': [
       'cpp'
      ,'cpp/bevstream'
      ,"<!(node -e \"require('nan')\")"
      ]

      ,"libraries": [ "/usr/local/lib/libevent.so" ]
    }


    ,{
      'target_name': 'TurnstileChain'
      ,'cflags': [ '-Werror=return-type' ]
      ,'cflags_cc': [ '-Werror=return-type' ]

      ,'sources':[
       'cpp/TurnstileChain.cpp'
      ,'cpp/bevstream/BevPair2.cpp'
      ,'cpp/bevstream/BevStream.cpp'
      ,'cpp/bevstream/Turnstile.cpp'
      ,'cpp/bevstream/ToJs.cpp'
      ]

      ,'include_dirs': [
       'cpp'
      ,'cpp/bevstream'
      ,"<!(node -e \"require('nan')\")"
      ]

      ,"libraries": [ "/usr/local/lib/libevent.so" ]
    }


    ,{
      'target_name': 'CoarseChain'
      ,'cflags': [ '-Werror=return-type' ]
      ,'cflags_cc': [ '-Werror=return-type' ]

      ,'sources':[
       'cpp/CoarseChain.cpp'
      ,'cpp/bevstream/BevPair2.cpp'
      ,'cpp/bevstream/BevStream.cpp'
      ,'cpp/bevstream/CoarseSync.cpp'
      ,'cpp/bevstream/ToJs.cpp'
      ,'../src/common/convert.cpp'
      ]

      ,'include_dirs': [
       'cpp'
      ,'cpp/bevstream'
      ,'../src/common'
      ,'../src'
      ,"<!(node -e \"require('nan')\")"
      ]

      ,"libraries": [ "/usr/local/lib/libevent.so" ]
    }


    ,{
      'target_name': 'FFTChain'
      ,'cflags': [ '-Werror=return-type' ]
      ,'cflags_cc': [ '-Werror=return-type' ]

      ,'sources':[
       'cpp/FFTChain.cpp'
      ,'cpp/bevstream/BevPair2.cpp'
      ,'cpp/bevstream/BevStream.cpp'
      ,'cpp/bevstream/FFT.cpp'
      ,'cpp/bevstream/ToJs.cpp'
      ,'cpp/bevstream/IShortToIFloat64.cpp'
      ,'../src/common/convert.cpp'
      ]

      ,'include_dirs': [
       'cpp'
      ,'cpp/bevstream'
      ,'../src/common'
      ,'../src'
      ,'/usr/local/include/eigen3/'
      ,"<!(node -e \"require('nan')\")"
      ]

      ,"libraries": [ "/usr/local/lib/libevent.so" ]
    }


    ,{
      'target_name': 'TxChain1'
      ,'cflags': [ '-Werror=return-type' ]
      ,'cflags_cc': [ '-Werror=return-type' ]

      ,'sources':[
       'cpp/TxChain1.cpp'
      ,'cpp/bevstream/BevPair2.cpp'
      ,'cpp/bevstream/BevStream.cpp'
      ,'cpp/bevstream/FFT.cpp'
      ,'cpp/bevstream/ToJs.cpp'
      ,'cpp/bevstream/IShortToIFloat64.cpp'
      ,'../src/common/convert.cpp'
      ]

      ,'include_dirs': [
       'cpp'
      ,'cpp/bevstream'
      ,'../src/common'
      ,'../src'
      ,'/usr/local/include/eigen3/'
      ,"<!(node -e \"require('nan')\")"
      ]

      ,"libraries": [ "/usr/local/lib/libevent.so" ]
    }


    ,{
      'target_name': 'RxChain1'
      ,'cflags': [ '-Werror=return-type' ]
      ,'cflags_cc': [ '-Werror=return-type' ]

      ,'sources':[
       'cpp/RxChain1.cpp'
      ,'cpp/bevstream/BevPair2.cpp'
      ,'cpp/bevstream/BevStream.cpp'
      ,'cpp/bevstream/FFT.cpp'
      ,'cpp/bevstream/ToJs.cpp'
      ,'cpp/bevstream/IShortToIFloat64.cpp'
      ,'cpp/bevstream/IFloat64ToIShort.cpp'
      ,'cpp/bevstream/CoarseSync.cpp'
      ,'cpp/bevstream/Turnstile.cpp'
      ,'../src/common/convert.cpp'
      ]

      ,'include_dirs': [
       'cpp'
      ,'cpp/bevstream'
      ,'../src/common'
      ,'../src'
      ,'/usr/local/include/eigen3/'
      ,"<!(node -e \"require('nan')\")"
      ]

      ,"libraries": [ "/usr/local/lib/libevent.so" ]
    }



  ]
}