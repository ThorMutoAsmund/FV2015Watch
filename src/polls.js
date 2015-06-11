var xhrRequest = function (url, type, callback) {
  var xhr = new XMLHttpRequest();
  xhr.onload = function () {
    callback(this.responseText);
  };
  xhr.open(type, url);
  xhr.send();
};

function sendPollInfo(redBlock, blueBlock) {
  // Assemble dictionary using our keys
  var dictionary = {
    'KEY_POLL_RED_BLOCK': ''+redBlock,
    'KEY_POLL_BLUE_BLOCK': ''+blueBlock
  };
  
  // Send to Pebble
  Pebble.sendAppMessage(dictionary,
    function(e) {
      console.log('Poll info sent to Pebble successfully!');
    },
    function(e) {
      console.log('Error sending poll info to Pebble!');
    }
  );
}

function getPolls() {
  // Construct URL
  var url = 'http://www.b.dk/upload/webred/bmsandbox/opinion_poll/2015/pollofpolls.xml';

  // Send request
  xhrRequest(url, 'GET', 
    function(xml) {
      var redBlock = 0;
      var blueBlock = 0;
      
      var entriesString = xml.match(/<entries>([\s\S]+?)<\/entries>/i);
      var entries = entriesString[1].match(/<entry>([\s\S]+?)<\/entry>/ig);
      if (entries !== null) {
        for (var i in entries) {
          var letter = entries[i].match(/<letter>([\s\S]+?)<\/letter>/i);
          var percent = entries[i].match(/<percent>([\s\S]+?)<\/percent>/i);
          if (letter !== null && percent !== null) {
            letter = letter[1];
            percent = percent[1];
            if (letter == 'A' || letter == 'B' || letter == 'F' || letter == 'Ø' || letter == 'Å') {
              redBlock += parseFloat(percent);
            }
            else if (letter == 'V' || letter == 'O' || letter == 'I' || letter == 'C' || letter == 'K') {
              blueBlock += parseFloat(percent);
            }
          }
        }
        
        sendPollInfo(redBlock, blueBlock);
        console.log('Red block: ' + redBlock + ' - Blue block: ' + blueBlock);
      }
    }      
  );
}

// Listen for when the watchface is opened
Pebble.addEventListener('ready', 
  function(e) {
    console.log('PebbleKit JS ready!');

    // Get the initial weather
    getPolls();
  }
);

// Listen for when an AppMessage is received
Pebble.addEventListener('appmessage',
  function(e) {
    console.log('AppMessage received!');
    getPolls();
  }                     
);

/*
var partyData = [];

var get_xml = function (year) {
    $.ajax({
        type: "GET",
        url: '../' + year + '/pollofpolls.xml',
        dataType: "xml",
        cache: false,
        success: function (xml) {
            var poll = $(xml).find("poll:first").find("entry").sort(function (a, b) {
                var d2 = $(a).find("party").find("letter").text();
                var d1 = $(b).find("party").find("letter").text();

                return (d1 < d2 ? -1 : (d1 > d2 ? +1 : 0));
            });

            var pollDateTime = $(xml).find("poll:first").find("datetime:first").text().split(' ');
            var pollDate = pollDateTime[0].split('-');

            generate_view(poll, pollDate);
        },
        error: function () {
            get_xml(year - 1);
        }
    });
};
*/