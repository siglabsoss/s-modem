

if( false ) {
  thebuffer = new ArrayBuffer(4); // in bytes
  // uint8_view = new Uint8Array(thebuffer);
  uint32_view = new Uint32Array(thebuffer);
  // float64_view = new Float64Array(thebuffer);

  uint32_view[0] |= 0xff << 24;
  uint32_view[0] |= 0xffffff;



  console.log(uint32_view[0]);
  console.log(uint32_view[0].toString(16));
  console.log('--');

  uint32_view[0]++;

  console.log(uint32_view[0]);
  console.log(uint32_view[0].toString(16));

}

if( false ) {
  var target = {}
  var handler = {}
  var proxy = new Proxy(target, handler)
  proxy.a = 'b'
  console.log(target.a)
  // <- 'b'
  console.log(proxy.c === undefined)
}

if( true ) {
  var handler = {
    get (target, key) {
      invariant(key, 'get')
      return target[key]
    },
    set (target, key, value) {
      invariant(key, 'set')
      target[key] = value;
      return true
    }
  }
  function invariant (key, action) {
    if (key[0] === '_') {
      throw new Error(`Invalid attempt to ${action} private "${key}" property`)
    }
  }
  var target = {}
  var proxy = new Proxy(target, handler);
  proxy.a = 'b';
  console.log(proxy.a)
  // <- 'b'
  // proxy._prop
  // // <- Error: Invalid attempt to get private "_prop" property
  // proxy._prop = 'c'
  // <- Error: Invalid attempt to set private "_prop" property
}