
const ChartjsNode = require('chartjs-node');
const _ = require('lodash');

// x
// config.data.datasets[0].data

// y
// config.data.labels

// config.options.title.text
// config.options.scales.yAxes[0].ticks.min
// config.options.scales.yAxes[0].ticks.max

let config = {
  type: 'line',
  data: {
    labels: [],
    datasets: [{
      label: 'Line',
      backgroundColor: '#ffffff', // not sure
      borderColor: '#000000',
      data: [],
      fill: false,
      lineTension:0,
      pointRadius:0,
      borderWidth:1
    }]
  },
  options: {
    responsive: true,
    title: {
      display: true,
      text: 'Title'
    },
    scales: {
      yAxes: [{
        ticks: {
          min: 0,
          max: 100
        }
      }]
    }

    ,chartArea: {
        backgroundColor: 'rgba(252, 255, 220, 1.0)'
    }
    ,image: {
        backgroundColor: 'rgba(255, 255, 255, 1.0)'
    }

  },

  plugins: {
    beforeDraw: function (chart, easing) {
        if (chart.config.options.chartArea && chart.config.options.chartArea.backgroundColor) {
            var helpers = ChartjsNode.helpers;
            var ctx = chart.chart.ctx;
            var chartArea = chart.chartArea;

            // console.log(JSON.stringify(ctx));
            // console.log(JSON.stringify(chartArea));

            // dims of image
            var w = parseInt(ctx.canvas.$chartjs.initial.width);
            var h = parseInt(ctx.canvas.$chartjs.initial.height);

            // var fill = {left:0,top:0,right:w,bottom:h};
            // console.log(JSON.stringify(fill));

            // console.log(chartArea.left);
            // console.log(chartArea.top)
            // console.log(chartArea.right - chartArea.left)
            // console.log(chartArea.bottom - chartArea.top)
            ctx.save();
            ctx.fillStyle = chart.config.options.image.backgroundColor;
            ctx.fillRect(0, 0, w, h);

            ctx.fillStyle = chart.config.options.chartArea.backgroundColor;
            ctx.fillRect(chartArea.left, chartArea.top, chartArea.right - chartArea.left, chartArea.bottom - chartArea.top);
            ctx.restore();
        }
    }
  }
};





// 600x600 canvas size
var chartNode = new ChartjsNode(1024, 600);







function plot(data, fn) {

// config.options.scales.yAxes[0].ticks.min
// config.options.scales.yAxes[0].ticks.max

  let datamin = _.min(data);
  let datamax = _.max(data);

  // console.log('min ' + datamin);

  let yvals = [];

  for(let i = 0; i < data.length; i++) {
    // data.push(Math.random()*100);
    yvals.push(i);
  }

  // modify the shared config object
  config.data.datasets[0].data = data;
  config.data.labels = yvals;
  config.options.scales.yAxes[0].ticks.min;
  config.options.scales.yAxes[0].ticks.max;
  config.options.scales.yAxes[0].ticks.min = datamin;
  config.options.scales.yAxes[0].ticks.max = datamax;


// x
// config.data.datasets[0].data

// y
// config.data.labels






  return chartNode.drawChart(config)
  .then(() => {
      // chart is created
   
      // get image as png buffer
      return chartNode.getImageBuffer('image/png');
  })
  .then(buffer => {
      Array.isArray(buffer) // => true
      // as a stream
      return chartNode.getImageStream('image/png');
  })
  .then(streamResult => {
      // using the length property you can do things like
      // directly upload the image to s3 by using the
      // stream and length properties
      streamResult.stream // => Stream object
      streamResult.length // => Integer length of stream
      // write to a file
      return chartNode.writeImageToFile('image/png', fn);
  })
  .then(() => {
      // chart is now written to the file path
      // ./testimage.png
  });

}


module.exports = {
    plot
}