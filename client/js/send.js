function SendRequestPOST(url, data, success)
{
	console.log('here');
	var xhttp = new XMLHttpRequest();
	xhttp.open('POST', url)
	xhttp.onreadystatechange = function(){
		if(this.readyState ==4 && this.status == 200){
			success(xhttp.responseText);
		}
	}
	xhttp.setRequestHeader('X-Requested-With', 'XMLHttpRequest');
	xhttp.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded');
	xhttp.send(data);
	return xhttp;
}

function SendRequestGET(url, success)
{
	var xhttp = new XMLHttpRequest();
	xhttp.onreadystatechange = function(){
		if(this.readyState > 3 && this.status == 200)
		{
			success(xhttp.responseText);
		}
	}

	xhttp.setRequestHeader('X-Requested-With', 'XMLHttpRequest');
	xhttp.send();
	return xhttp;
}
