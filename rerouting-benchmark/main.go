package main

import (
	"fmt"
	"io"
	"net/http"
	"encoding/json"
)

func Fibonacci(n int64) (int64, error) {

	if n <= 1 {
		return n, nil
	}

	if n > 93 {
		return 0, fmt.Errorf("unsupported fibonacci number %d: too large", n)
	}

	var n2, n1 int64 = 0, 1
	for i := int64(2); i < n; i++ {
		n2, n1 = n1, n1+n2
	}

	return n2 + n1, nil
}

func statesAcceptor(w http.ResponseWriter, r *http.Request) {
	fmt.Println("Accepting new request")

	for name, values := range r.Header {
    		// Loop over all values for the name.
    		for _, value := range values {
        		fmt.Println(name, value)
    		}
	}

	defer r.Body.Close()
	body, _ := io.ReadAll(r.Body) 

	fmt.Println(string(body))
	_, err := io.WriteString(w, "{}")
	if err != nil {
		return
	}
}

func main() {
	http.HandleFunc("/v3/discovery:listener", statesAcceptor);

	_ = http.ListenAndServe(":10901", nil);
}
