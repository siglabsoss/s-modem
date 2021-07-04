gnuplot = require('gnu-plot');

// gnuplot().plot([{
//     data:[[0,0],[1,1],[2,0]]
// }]);

function plot(x, options={}) {
  const data = [];

  let {title = null} = options;



  for(let i = 0; i < x.length; i++) {
    // data.push(Math.random()*100);
    data.push([i,x[i]]);
  }

  let chain = gnuplot();

  if( title !== null ) {
    chain.set({title:`"${title}"`});
  }

  // chain.set(['"nokey"']);
  // chain = chain.

  chain.plot([{
      data:data
  }]);


}

module.exports = {
    plot
};