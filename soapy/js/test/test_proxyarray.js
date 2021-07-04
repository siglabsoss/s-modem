const jsc = require("jsverify");
const assert = require('assert');
const mutil = require("../mock/mockutils.js");
const mt = require('mathjs');
const _ = require('lodash');
const proxyArray = require('../mock/proxyarray.js')




describe("misc", function () {



  it('proxy array', function(done) {

    let expected = [1,2,3,4,5];

    let cb = function(i) {
      // console.log(JSON.stringify(['hello',i]));
      return expected[i];
    }

    let asArray = proxyArray(cb, expected.length);

    // console.log(asArray.length);

    // console.log(asArray[0]);
    // console.log(asArray[10]);

    // asArray[0] = 1;

    let slc = asArray.slice(1,3);

    // console.log(slc);

    // asArray.forEach(function(element) {
    //   console.log(element);
    // });

    // let bar = [1,2,3,4];

    // const res = bar.map(x=>x*2);
    // console.log(res);

    // const map1 = asArray.map(x => x * 20);
    // const map1 = asArray.map((x,i,a) => { 
    //   return a[i];
    //  });

    // console.log(map1);

    // console.log(asArray.reverse()); // should throw


    done();

  });

  
  it('proxy array filter', function() {

    let expected = ['spray', 'limit', 'elite', 'exuberant', 'destruction', 'present'];

    let cb = function(i) {
      // console.log(JSON.stringify(['hello',i]));
      return expected[i];
    }

    let asArray = proxyArray(cb, expected.length);


    const result = asArray.filter(word => word.length > 6);

    console.log(result);


  });
  
  it('proxy array reduce', function() {

    let expected = [1, 2, 3, 4];

    let cb = function(i) {
      // console.log(JSON.stringify(['hello',i]));
      return expected[i];
    }

    let array1 = proxyArray(cb, expected.length);


    const reducer = (accumulator, currentValue) => accumulator + currentValue;

    // 1 + 2 + 3 + 4
    console.log(array1.reduce(reducer));
    // expected output: 10

    // 5 + 1 + 2 + 3 + 4
    console.log(array1.reduce(reducer, 5));
    // expected output: 15

  });

    it('proxy array reduce 2', function() {

    let expected = [[0, 1], [2, 3], [4, 5]];

    let cb = function(i) {
      // console.log(JSON.stringify(['hello',i]));
      return expected[i];
    }

    let array1 = proxyArray(cb, expected.length);


    var flattened = array1.reduce(
      function(accumulator, currentValue) {
        return accumulator.concat(currentValue);
      },
      []
    );

    console.log(flattened);

  });

  it('proxy array reduce 3', function() {

    let expected = ['Alice', 'Bob', 'Tiff', 'Bruce', 'Alice'];

    let cb = function(i) {
      // console.log(JSON.stringify(['hello',i]));
      return expected[i];
    }

    let names = proxyArray(cb, expected.length);

    var countedNames = names.reduce(function (allNames, name) { 
      if (name in allNames) {
        allNames[name]++;
      }
      else {
        allNames[name] = 1;
      }
      return allNames;
    }, {});

    console.log(countedNames);

  });

  it('proxy array every', function() {

    let expected = [1, 30, 39, 29, 10, 13];

    let cb = function(i) {
      // console.log(JSON.stringify(['hello',i]));
      return expected[i];
    }

    let array1 = proxyArray(cb, expected.length);

    function isBelowThreshold(currentValue) {
      return currentValue < 40;
    }

    console.log(array1.every(isBelowThreshold));
    // expected output: true

  });

  it('proxy array log', function() {

    let expected = ['Fire', 'Wind', 'Rain'];

    let cb = function(i) {
      // console.log(JSON.stringify(['hello',i]));
      return expected[i];
    }

    let elements = proxyArray(cb, expected.length);

    console.log(elements.join(undefined));
    // expected output: Fire,Wind,Rain

    console.log('----------');
    console.log(expected);
    console.log(elements);

  });
  

  it('proxy array keys', function() {

    let expected = ['Fire', 'Wind', 'Rain'];

    let cb = function(i) {
      // console.log(JSON.stringify(['hello',i]));
      return expected[i];
    }

    let elements = proxyArray(cb, expected.length);

    // console.log(elements);
    console.log(Array.from(expected.keys()));
    console.log(Array.from(elements.keys()));

    assert(_.isEqual(
      Array.from(expected.keys()),
      Array.from(elements.keys())
      ));

    assert(_.isEqual(
      Array.from(expected.entries()),
      Array.from(elements.entries())
      ));

    var expectedArr = expected[Symbol.iterator]();
    var elementsArr = elements[Symbol.iterator]();

    assert(_.isEqual(
      Array.from(expectedArr),
      Array.from(elementsArr)
      ));

    assert(_.isEqual(
      Array.from(expected.values()),
      Array.from(elements.values())
      ));


    

  });






}); // describe