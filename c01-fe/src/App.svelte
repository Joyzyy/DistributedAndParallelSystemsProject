<script lang="ts">
  let selectedOperation = $state('');
  let selectedMode: 'ECB' | 'CBC' | '' = $state('');
  let file: File | null = $state(null);
  let key: string = $state('');

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
        const response = await fetch('http://localhost:8080/api/recv-img', {
          method: "POST",
          headers: {
            'Content-Type': 'application/json'
          },
          body: JSON.stringify({
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

        const result = await response.json();
        console.info(result.result);
    } catch (error) {
        console.error('Error uploading file:', error);
        alert('An error occurred while uploading the file.');
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
  </section>
</main>