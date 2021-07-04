'use strict';

function typeValid(t) {
  switch(t) {
    case 'any':
    case 'forward':
    case 'IShort':
    case 'IFloat':
      return true;
      break;
    default:
      return false;
  }
}

function chunkCompatible(us, them) {
  if( us === 'any' ) {
    return true;
  }
  return false;
}


function check(us, them, print=true) {
  if( typeof us.pipeType === 'undefined' ) {
    if( print ) {
      console.log('us does not have pipeType member');
    }
    return ['na', 'us missing information'];
  }

  // if( !typeValid(us.pipeType.in.type) || !typeValid(us.pipeType..type) )

  if( typeof them.pipeType === 'undefined' ) {
    if( print ) {
      console.log('us does not have pipeType member');
    }
    return ['na', 'them missing information'];
  }

  if( print ) {
    console.log(us.name);
    console.log(us.pipeType);
    console.log(them.name);
    console.log(them.pipeType);
  }

  if( us.pipeType.in.type === them.pipeType.out.type
     && chunkCompatible(us.pipeType.in.chunk, them.pipeType.out.chunk) 
     ) {
    return ['ok',''];
  }

  return ['error','?'];

}

function error(us, them, print=true) {

}

function warn(us, them, print=true) {
  let [res, why] = check(us, them, print);

  let because = '';
  if( why !== '' ) {
    because = ' because ' + why;
  }

  if( res !== 'ok' ) {
    console.log("PipeTypeCheck result was " + res + because);
  }
}

module.exports = {
    warn
  , error
  , check
}