const fs = require('fs');
const _ = require('lodash');
const util = require('util');

// Each line here must be a complete type
// t: the type in c++
// js: the js type which most closly matches this
// comb: how to convert into the type
let inputTypes = [
 {t:'int',js:'Napi::Number',check:'IsNumber',conv:'As<Napi::Number>().Int32Value()'}
,{t:'int32_t',js:'Napi::Number',check:'IsNumber',conv:'As<Napi::Number>().Int32Value()'}
,{t:'uint32_t',js:'Napi::Number',check:'IsNumber',conv:'As<Napi::Number>().Uint32Value()'}
,{t:'unsigned',js:'Napi::Number',check:'IsNumber',conv:'As<Napi::Number>().Uint32Value()'}
,{t:'ssize_t',js:'Napi::Number',check:'IsNumber',conv:'As<Napi::Number>().Int64Value()'}
,{t:'size_t',js:'Napi::Number',check:'IsNumber',conv:'As<Napi::Number>().Int64Value()'}
,{t:'int64_t',js:'Napi::Number',check:'IsNumber',conv:'As<Napi::Number>().Int64Value()'}
,{t:'uint64_t',js:'Napi::Number',check:'IsNumber',conv:'As<Napi::Number>().Int64Value()'}
,{t:'double',js:'Napi::Number',check:'IsNumber',conv:'As<Napi::Number>()'}
,{t:'bool',js:'Napi::Boolean',check:'IsBoolean',conv:'As<Napi::Boolean>()'}
,{t:'string',js:'Napi::String',check:'IsString',conv:'As<Napi::String>()'}
,{t:'void',js:'void',check:'',conv:''}
];

// maximum number of combinations
let maxArguments = 4;



// Below this line are calculated values

let varArgsCount = maxArguments + 5;


let inputTypesNoVoid = _.clone(inputTypes);

let outputTypes = _.clone(inputTypes);
let outputTypesNoVoid = _.clone(outputTypes);

let typePullFilter = (v1,v2) => {
  return v1.t === v2.t;
}

_.pullAllWith(outputTypesNoVoid,[{t:'void'}],typePullFilter);
_.pullAllWith(inputTypesNoVoid,[{t:'void'}],typePullFilter);

let inputCombinations = [];

// for(let x of inputTypesNoVoid) {
//   for(let y of inputTypesNoVoid) {
//     console.log(x.t + ' - ' + y.t);
//   }
// }

let g_comb = [];

// This function simulates the body of a nested for loop
// where the number of nested loops is variable.  
// for instance, a snip of what this gets called with might look like:
//
//   [ 0, 5, 5 ]
//   [ 0, 5, 6 ]
//   [ 0, 6, 0 ]
//   [ 0, 6, 1 ]
//   [ 0, 6, 2 ]
//   [ 0, 6, 3 ]
//   [ 0, 6, 4 ]
//   [ 0, 6, 5 ]
//   [ 0, 6, 6 ]
//   [ 1, 0, 0 ]

// this function filters any combinations that have a non leading zero
// a combination with a leading zero that is followed by a zero is kept
// the values of all outputs are decremented by 1, so that zeros appear in final output
function combTarget(v1) {
  // console.log(v1);
  let filtered = [];  // a filtered version of v1
  let i = 0;
  let skip = false;
  for(let x of v1) { // loop through each index
    if( x ) {
      filtered.push(x-1); // always save non zero entries
    } else {
      if(filtered.length) {
        skip = true;  // if this combination has zeros that are not leading, drop entire combination
        break;
      }
    }
    i++;
  }

  if( !skip && filtered.length) {
    g_comb.push(filtered);
    // console.log(v2);
  }
}

// https://stackoverflow.com/questions/4683539/variable-amount-of-nested-for-loops
function doCallManyTimes(maxIndices, func, args, index) {
    if (maxIndices.length == 0) {
        func(args);
    } else {
        var rest = maxIndices.slice(1);
        for (args[index] = 0; args[index] < maxIndices[0]; ++args[index]) {
            doCallManyTimes(rest, func, args, index + 1);
        }
    }
}
function callManyTimes(maxIndices, func) {
    doCallManyTimes(maxIndices, func, [], 0);
}

let nonVoidCount = inputTypesNoVoid.length;
let combArg = [];
for(let i = 0; i < maxArguments; i++) {
  combArg.push(nonVoidCount+1);
  // console.log(i);
}

// console.log(combArg);
callManyTimes(combArg, combTarget);

// at this point g_comb has all permutations of inputs we would like to call

// for(let x of g_comb) {
//   console.log(x);
// }
// console.log(g_comb);

function bulidComb(inputs, comb) {
  let ret = [];
  for( let x of comb ) {
    // console.log('-------');
    let o = {path:''};
    let len = x.length;
    
    for(let i = 0; i < x.length; i++) {
      let hive = inputs[x[i]];
      // console.log('  '.repeat(i) + inputs[x[i]].t);
      o.path += '_' + hive.t;
    }

    let validateError = ' expects argument' + (x.length==1?'':'s') +  ' of (';
    let validateArgs = '';
    let validate = `bool call_notok = info.Length() != ${len}`;

    for(let i = 0; i < x.length; i++) {
      let hive = inputs[x[i]];
      validate += ` || !info[${i}].${hive.check}()`;
      validateArgs += ' ' + hive.t + ',';
    }
    validateArgs = validateArgs.trimStart();
    validateArgs = _.trim(validateArgs, ',');
    validateError += validateArgs;
    validateError += ')';
    validate += '; \\\n';
    validate += `if(call_notok){Napi::TypeError::New(env, __STR(fn) "${validateError}").ThrowAsJavaScriptException();}\n`;

    o.validate = validate;
    o.ins = [];
    for(let i = 0; i < x.length; i++) {
      o.ins.push(inputs[x[i]]);
    }
    // console.log(validate);



    ret.push(o);
  }
  return ret;
}


let combTemplate = bulidComb(inputTypesNoVoid, g_comb);

let combDebug = [];

combDebug.push(combTemplate[0]);
combDebug.push(combTemplate[1]);
combDebug.push(combTemplate[14]);
combDebug.push(combTemplate[20+3]);
combDebug.push(combTemplate[21+3]);
combDebug.push(combTemplate[22+3]);
combDebug.push(combTemplate[96]);
combDebug.push(combTemplate[97]);
combDebug.push(combTemplate[98]);


let varArgs = [];
for(let i = 0; i < varArgsCount; i++) {
  varArgs.push(i);
}

// console.log(combTemplate);


// console.log(JSON.stringify(combDebug));
// console.log(inputTypes);
// console.log(outputTypes);
// console.log(outputTypesNoVoid);



let args = {ins:inputTypes,outs:outputTypes,outsNoVoid:outputTypesNoVoid,comb:combTemplate,vargs:varArgs};


// open the template
let fname;
fname = './templates/gen_NapiHelper.txt';
let sigs_header_file = fs.readFileSync(fname);



// generate template output from input
let sigs_header_compiled = _.template(sigs_header_file)(args);


console.log('----------Creating output------------');

fs.writeFile('out/NapiHelper.hpp',sigs_header_compiled, (err) => {
  if(err) {
    return console.log(err);
  }
});


