import React, { useState } from 'react';
import axios from 'axios';

function App() {
    const [data, setData] = useState('');
    const [orderId, setOrderId] = useState(1);

    const sendData = () => {
      	setOrderId(orderId + 1);
      	axios.post('http://localhost:8080', `S ${orderId} 105 1 GoodTillCancel`)
			.then(response => {
				console.log("Server response:", response.data);
				setData(response.data);
			})
			.catch(error => {
				console.error("There was an error making the request!", error);
			});
    };

    return (
        <div className="App">
            <header className="App-header">
                <h1>Orderbook Data</h1>
                <pre>{data}</pre>
                <button onClick={sendData}>Send Input Data</button>
            </header>
        </div>
    );
}

export default App;
