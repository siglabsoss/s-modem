// Based on MIT:
// https://github.com/munrocket/overload-bracket

'use strict';

/**
 * This class overloads operator[] and array methods to given object.
 * For example you have object with vector of points `let V = {points: [{x: 1, y: 2}, {x: 10, y: 20}, {x: -1, y: -2}]}`.
 * You can create pseudo-array of x points by `let X = new ObjectHandler(V, x => x.points, x => x.x, (x, v) => x.x = v`.
 * And access the elements of an pseudo-array like it ordinary array `(X[0] + X[1]) / X.length` and `X.filter(func)`
 */
class ProxyArray {

  /**
   * @constructor ObjectHandler constructor
   * @param object Target object with data
   * @param container Function that return iterable container
   * @param getter Getter inside container
   * @param setter Setter indide container
   */
  constructor(cb, length) {
    this.proxy = new Proxy([], this.handler(cb));
    this.save_length = length;
    return this.proxy;
  }

  handler(getter, setter) {
    return {

      get: (object, key) => {
        if (key === 'length') {
          return this.save_length;
        } else if (typeof Array.prototype[key] == 'function') { //array methods
          if (typeof this[key] == 'function') {
            return this[key]();
          } else if(key.toString() === 'Symbol(Symbol.iterator)') {
            return this.iterator();
          } else {
            return this.emulateArrayMethod([], key, [], getter);
          }
        } else { 
          try {                                               //access array by index
            if(key === parseInt(key).toString()) {
              if(0 <= key && key < this.save_length) {
                return getter(parseInt(key));
              } else {
                throw "index out of bondary";
              }
            } else {
              throw "float index";
            }
          } catch (err) {                                   //access to object by literal
            return Reflect.get(object, key);
          }
        }
      },

      set: (object, key, value) => {
        throw(new Error("proxy is readonly"));
        return false;
      }

    }
  }

  emulateArrayMethod(object, key, container, getter) {
    return (...args) => {
      try {
        console.log("Deprecated emulation. Better to define method " + key.toString() + "() by hand.");
        return Reflect.apply([][key], [], args).map(x => getter(x));
      } catch (err) {
        throw "Array method " + key.toString() + " can not be emulated!";
      }
    }
  }

  get length() {
    return this.proxy.length;
  }

  keys() {
    let gen = function*() {
      let i = 0;
      while(i < this.save_length) {
        yield i++;
      }
    };
    return gen.bind(this);
  }

  entries() {
    let gen = function*() {
      let i = 0;
      while(i < this.save_length) {
        yield [i, this.proxy[i]];
        i++;
      }
    };
    return gen.bind(this);
  }

  values() {
    let gen = function*() {
      let i = 0;
      while(i < this.save_length) {
        yield this.proxy[i];
        i++;
      }
    };
    return gen.bind(this);
  }

  iterator() {
    let gen = function*() {
      let i = 0;
      while(i < this.save_length) {
        yield this.proxy[i];
        i++;
      }
    };
    return gen.bind(this);
  }

  slice() {
    return (start, end = this.length) => {
      let result = [];
      for (let i = start; i < end; i++) {
        result.push(this.proxy[i]);
      }
      return result;
    }
  }

  forEach() {
    return (func) => {
      for(let i = 0; i < this.length; i++) {
        func(this.proxy[i], i, this.proxy);
      }
      return this.proxy;
    }
  }

  map() {
    return (op) => {
      let result = [];
      for (let i = 0; i < this.length; i++) {
        result[i] = op(this.proxy[i], i, this.proxy);
      }
      return result;
    }
  }

  filter() {
    return (op) => {
      let result = [];
      for (let i = 0; i < this.length; i++) {
        if (op(this.proxy[i], i, this.proxy)) {
          result.push(this.proxy[i]);
        }
      }
      return result;
    }
  }

  reduce() {
    return (op, init=0) => {
      let total = init;
      for (let i = 0; i < this.length; i++) {
        total = op(total, this.proxy[i], i, this.proxy);
      }
      return total;
    }
  }
  
  every() {
    return (op) => {
      let isEvery = true;
      for(let i = 0; i < this.length; i++) {
        if (op(this.proxy[i], i, this.proxy)) {
          continue;
        } else {
          isEvery = false;
          break;
        }
      }
      return isEvery;
    }
  }

  reverse() {
    return () => {
      for (let i = 0; i < Math.floor(this.length / 2); i++) {
        let temp = this.proxy[i];
        this.proxy[i] = this.proxy[this.length - i - 1];
        this.proxy[this.length - i - 1] = temp;
      }
      return this.proxy;
    }
  }

  join() {
    return (separator) => {
      let result = "";
      if( typeof separator === 'undefined' ) {
        separator = ',';
      }
      for (let i = 0; i < this.length - 1; i++) {
        result += this.proxy[i] + separator;
      }
      return result + this.proxy[this.length - 1];
    }
  }

  toString() {
    return () => {
      let result = "[ ";
      for (let i = 0; i < this.length - 1; i++) {
        result += this.proxy[i] + ", ";
      }
      return result + this.proxy[this.length - 1] + " ]";
    }
  }

  get [Symbol.toStringTag]() {
    return "ObjectHandler";
  }
}

function wrap(x,y) {
  return new ProxyArray(x,y);
}

module.exports = wrap;