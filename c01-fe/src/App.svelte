<script lang="ts">
  let selectedOperation = $state('');
  let selectedMode: 'ECB' | 'CBC' | '' = $state('');
  let file: File | null = $state(null);
  let key: string = $state('');
  let responseFromServer: string | null = $state(null);
  let closedWSConn: boolean | null = $state(null);

  let ws = $state<WebSocket | null>(null);
  $effect(() => {
    if (!ws || ws.readyState === WebSocket.CLOSED) {
      ws = new WebSocket("ws://localhost:7777/ws");
      ws.onopen = () => {
        console.log("WebSocket connection established");
        console.log("Setting userId to localStorage");
        if (!localStorage.getItem("userId")) {
          console.log("No userId found in localStorage, generating a new one");
          const userId = Math.random().toString(36).substring(2, 15);
          localStorage.setItem("userId", userId);
          ws.send(JSON.stringify({ action: "setUserId", data: userId }));
        } else {
          console.log("Found userId in localStorage:", localStorage.getItem("userId"));
          ws.send(JSON.stringify({ action: "setUserId", data: localStorage.getItem("userId") }));
        }
      };
      ws.onclose = () => {
        closedWSConn = true;
        console.log("WebSocket connection closed");
      };
      ws.onerror = (error) => {
        console.error("WebSocket error:", error);
      };
      ws.onmessage = (event) => {
        console.log("message received from server:", event.data);
        responseFromServer = event.data;
      }
    }
    
    return () => {
      if (ws && ws.readyState === WebSocket.OPEN) {
        ws.close();
      }
    }
  });

  const AVAILABLE_MODES = ['ECB', 'CBC'];
  const AVAILABLE_OPERATIONS = [{
    name: "Encrypt",
    icon: "🔒",
    id: "encrypt",
  }, {
    name: "Decrypt",
    icon: "🔑",
    id: "decrypt",
  }];

  function handleFileChange(event: Event) {
    const input = event.target as HTMLInputElement;
    if (input.files && input.files.length > 0) {
      file = input.files[0];
    }
  }

  function convertToBase64(file: File): Promise<string> {
    return new Promise((resolve, reject) => {
      const reader = new FileReader();
      reader.onload = () => resolve(reader.result as string);
      reader.onerror = error => reject(error);
      reader.readAsDataURL(file);
    });
  }

  async function handleSubmit(event: Event) {
    event.preventDefault();
    if (!file) {
      alert('Please upload a file to encrypt.');
      return;
    }

    const base64Image = await convertToBase64(file);

    try {
        const response = await fetch('http://localhost:7777/api/recv-img', {
          method: "POST",
          headers: {
            'Content-Type': 'application/json'
          },
          body: JSON.stringify({
            userId: localStorage.getItem("userId"),
            imageName: file.name,
            base64Image: base64Image,
            operation: selectedOperation,
            mode: selectedMode,
            key: key
          })
        });

        if (!response.ok) {
          throw new Error('Network response was not ok');
        }
    } catch (error) {
        console.error('Error uploading file:', error);
    }
  }

  const downloadImageFromServer = () => {
    if (responseFromServer) {
      const link = document.createElement('a');
      link.href = `http://localhost:3000/download-file/${localStorage.getItem("userId")}/${responseFromServer.split(",")[1]}`;
      link.click();
      link.remove();
    }
  }
</script>

<main class="h-[100vh] flex items-center justify-center bg-gray-100">
  <section class="mx-auto container max-w-md bg-white rounded-lg shadow p-6">
    <h1 class="text-2xl font-bold mb-4">Encrypt and decrypt your files</h1>
    <form class="space-y-4" onsubmit={handleSubmit}>
      <div>
        <label class="block mb-1 font-medium" for="inputFile">Select File</label>
        <input
          id="inputFile"
          type="file"
          class="block w-full text-sm text-gray-700 border border-gray-300 rounded px-3 py-2 focus:outline-none focus:ring-2 focus:ring-blue-500"
          placeholder="Choose a file"
          onchange={handleFileChange}
        />
      </div>
      <div>
        <label class="block mb-1 font-medium" for="operationSelect">Operation</label>
        <select
          id="operationSelect"
          bind:value={selectedOperation}
          class="block w-full border border-gray-300 rounded px-3 py-2 focus:outline-none focus:ring-2 focus:ring-blue-500"
          required
        >
          <option value="" disabled selected>Select an operation</option>
          {#each AVAILABLE_OPERATIONS as operation}
            <option value={operation.id}>{operation.icon} {operation.name}</option>
          {/each}
        </select>
      </div>
      <div>
        <label class="block mb-1 font-medium" for="algorithmSelect">Mode (CBC/ECB)</label>
        <select
          id="algorithmSelect"
          bind:value={selectedMode}
          class="block w-full border border-gray-300 rounded px-3 py-2 focus:outline-none focus:ring-2 focus:ring-blue-500"
          required
        >
          <option value="" disabled selected>Select a mode</option>
          {#each AVAILABLE_MODES as mode}
            <option value={mode}>{mode}</option>
          {/each}
        </select>
      </div>
      <div>
        <label class="block mb-1 font-medium" for="key">Symmetric key</label>
        <input type="text" id="key" placeholder="Enter symmetric key" class="block w-full border border-gray-300 rounded px-3 py-2 focus:outline-none focus:ring-2 focus:ring-blue-500" required 
          bind:value={key}
        />
      </div>
      <button
        type="submit"
        class="w-full bg-blue-600 text-white py-2 rounded hover:bg-blue-700 transition-colors duration-200"
      >
        Upload{""}
        {#if selectedOperation === 'encrypt'}
          and Encrypt
        {:else if selectedOperation === 'decrypt'}
          and Decrypt
        {/if}
        {#if selectedMode}
          in {selectedMode} mode
        {/if}
      </button>
    </form>

    {#if responseFromServer}
      <div class="mt-4 text-green-600">
        <p>{responseFromServer}</p>
      </div>
      <button 
        class="mt-2 bg-gray-200 text-gray-800 px-4 py-2 rounded hover:bg-gray-300 transition-colors duration-200" 
        onclick={() => downloadImageFromServer()}
      >
        Download image
      </button>
    {/if}
    {#if closedWSConn}
      <div class="mt-4 text-red-600">
        <p>WS conn has failed (need to wait for javalin server to start...), please reload the page!</p>
      </div>
    {/if}
  </section>
</main>