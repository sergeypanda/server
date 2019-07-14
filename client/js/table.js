var obj;
var API_URL = 'http://localhost:20000';

var table = new Tabulator("#example-table", {
    height:"250px",

    responsiveLayout:"hide",
    columns:[
    {title:"Name", field:"name", width:200, responsive:0}, //never hide this column
    {title:"Progress", field:"progress", align:"right", sorter:"number", width:150},
//	    {title:"Gender", field:"gender", width:150, responsive:2}, //hide this column first
//	    {title:"Rating", field:"rating", align:"center", width:150},
    {title:"Favourite Color", field:"col", width:150},
//	    {title:"Date Of Birth", field:"dob", align:"center", sorter:"date", width:150},
    {title:"Driver", field:"car", align:"center", width:150},
    ],
});


var form = document.querySelector('#addForm form');
const formData = new FormData(form);
console.log(formData.get('Driver'));

document.getElementById('sendPostRequest').addEventListener('click', function(){

var xhttp = SendRequestPOST(API_URL, '', function(resp){
//		var content = document.getElementById('content');
//		content.innerHTML = JSON.parse(resp).message;
  obj = JSON.parse(resp);
  console.log(resp);
  //define some sample data
   var tabledata = [
    {id:1, name:"Oli Bob", age:"12", col:"red", dob:""},
    {id:2, name:"Mary May", age:"1", col:"blue", dob:"14/05/1982"},
    {id:3, name:"Christine Lobowski", age:"42", col:"green", dob:"22/05/1982"},
    {id:4, name:"Brendon Philips", age:"125", col:"orange", dob:"01/08/1980"},
    {id:5, name:"Margret Marmajuke", age:"16", col:"yellow", dob:"31/01/1999"},
   ];
   table.setData(tabledata).then(function(){
     console.log('success!');
   }).catch(function(error){
     //handle error here
   });

});
});

document.querySelector('#newsletter button').addEventListener('click', function(){
  fetch(API_URL, {
    method: 'POST',
    cache: 'no-cache',
    credentials: 'same-origin',
    headers : {
      'Content-Type': 'application/json'
    },
    body: JSON.stringify(form)
  })
  .then(response => response.json())
  .then(res => {
    console.log('Success:', JSON.stringify(res));
  })

  .catch(error => console.error('Error:', error));
});
