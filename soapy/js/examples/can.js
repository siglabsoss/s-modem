function bplot() {
    return new Promise(function(resolve, reject) {
      
     setTimeout(function() {console.log('exit');resolve(true)}, 500);
    });
}

async function aplot() {
    console.log('enter');

    let val = await bplot();

}

function cplot() {
    let done = false;
    aplot().then(()=>done=true);
    while(!done){}
}


cplot();
cplot();
cplot();