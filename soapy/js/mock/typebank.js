// var handler = {
//   get (target, key) {
//     invariant(key, 'get')
//     return target[key]
//   },
//   set (target, key, value) {
//     invariant(key, 'set')
//     return true
//   }
// };
// function invariant (key, action) {
//   if (key[0] === '_') {
//     throw new Error(`Invalid attempt to ${action} private "${key}" property`)
//   }
// }
// var target = {}
// var proxy = new Proxy(target, handler)
// proxy.a = 'b'
// console.log(proxy.a)



class TypeBankUint32 {

  constructor() {

    this._opts = {};

    this._typesize = 4;

    this._handler = {};

    this._handler.get = (target,key) => {
      // this._invariant(key, 'get');

      if( target.type.hasOwnProperty(key) ) {
        // key exists
      } else {
        // fresh key, first action was to get
        this._createType(target, key);
        target.type[key][0] = 0; // default value when first action is to get
      }

      return target.type[key][0];

    }

    this._handler.set = (target, key, value) => {
      // this._invariant(key, 'set');

      // console.log('type size ' + this._typesize);

      if( target.type.hasOwnProperty(key) ) {
        // key exists
      } else {
        // fresh key, first action was to set
        this._createType(target, key);
      }

      target.type[key][0] = value;

      return true;
    }

    this._target = {buffer:{},type:{}};

    this._proxy = new Proxy(this._target, this._handler);

  } // constructor

  _createType(target, key) {
    target.buffer[key] = new ArrayBuffer(this._typesize);
    target.type[key] = new Uint32Array(target.buffer[key]);
  }

  // _invariant (key, action) {
  //   if (key[0] === '_') {
  //     throw new Error(`Invalid attempt to ${action} private "${key}" property`)
  //   }
  // }

  access() {
    return this._proxy;
  }


}

module.exports = {
    TypeBankUint32
}
